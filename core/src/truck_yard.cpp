#include "ds/truck_yard.h"

#include <cmath>
#include <cstdarg>
#include <cstdio>

namespace ds {

bool TruckYard::configure(int capacity, int tempCapacity, double feePerHour) {
    if (capacity <= 0 || tempCapacity < 0 || !std::isfinite(feePerHour) ||
        feePerHour < 0.0) {
        m_configured = false;
        return false;
    }

    m_config.capacity = capacity;
    m_config.tempCapacity = tempCapacity;
    m_config.feePerHour = feePerHour;

    m_trucks.clear();
    m_index.clear();
    m_channel.clear();
    m_temp.clear();
    for (int i = 0; i < 3; ++i) {
        m_queues[i].clear();
    }
    m_events.clear();
    m_eventSeq = 0;
    m_revenue = 0.0;
    m_arrived = 0;
    m_served = 0;
    m_maxTemp = 0;
    m_totalWait = 0.0;
    m_configured = true;

    log(0.0, "系统启动：主通道容量 %d 辆，临时区容量 %d 辆，费率 %.2f 元/小时",
        m_config.capacity, m_config.tempCapacity, m_config.feePerHour);
    return true;
}

bool TruckYard::arrive(int id, CargoType type, double t, char* err, int errSize) {
    if (!m_configured) {
        setError(err, errSize, "系统尚未初始化");
        return false;
    }
    if (id <= 0) {
        setError(err, errSize, "货车编号必须为正整数");
        return false;
    }
    if (!(t >= 0.0) || t > 1e9) {
        setError(err, errSize, "到达时刻非法");
        return false;
    }
    if (m_index.contains(id)) {
        setError(err, errSize, "货车编号重复");
        return false;
    }

    Truck truck;
    truck.id = id;
    truck.type = type;
    truck.arriveTime = t;
    truck.state = TruckState::Waiting;
    m_trucks.push_back(truck);
    const int index = m_trucks.size() - 1;
    m_index.put(id, index);

    // 同优先级按到达时刻排队；同一时刻保持输入顺序（稳定插入）。
    Queue<int>& sameType = m_queues[static_cast<int>(type)];
    if (sameType.empty() ||
        m_trucks[sameType.at(sameType.size() - 1)].arriveTime <= t) {
        sameType.enqueue(index);  // 绝大多数情况是顺序输入，保持 O(1)
    } else {
        int insertPos = 0;
        while (insertPos < sameType.size() &&
               m_trucks[sameType.at(insertPos)].arriveTime <= t) {
            ++insertPos;
        }
        sameType.insertAt(insertPos, index);
    }
    ++m_arrived;

    log(t, "货车 %d（%s）到达，进入%s队列", id, cargoName(type),
        type == CargoType::Fresh ? "生鲜" : (type == CargoType::Normal ? "普通" : "大件"));

    fillChannel(t);
    return true;
}

bool TruckYard::depart(int id, double t, double* fee, double* dwell,
                       char* err, int errSize) {
    if (!m_configured) {
        setError(err, errSize, "系统尚未初始化");
        return false;
    }
    int index = -1;
    if (!m_index.get(id, index)) {
        setError(err, errSize, "货车不存在");
        return false;
    }
    Truck& truck = m_trucks[index];

    if (truck.state == TruckState::Left) {
        setError(err, errSize, "货车已离场");
        return false;
    }
    if (!std::isfinite(t) || t < truck.arriveTime) {
        setError(err, errSize, "离场时刻不能早于到达时刻");
        return false;
    }

    if (truck.state == TruckState::Waiting) {
        if (!removeFromQueue(index, truck.type)) {
            setError(err, errSize, "货车不在队列中");
            return false;
        }
        truck.state = TruckState::Left;
        truck.leaveTime = t;
        log(t, "货车 %d（%s）取消排队离场", id, cargoName(truck.type));
        if (fee != nullptr) {
            *fee = 0.0;
        }
        if (dwell != nullptr) {
            *dwell = 0.0;
        }
        fillChannel(t);
        return true;
    }

    if (truck.state == TruckState::Temp) {
        setError(err, errSize, "货车正在临时区，请稍后重试");
        return false;
    }

    const int pos = channelPos(index);
    if (pos < 0) {
        setError(err, errSize, "货车状态异常");
        return false;
    }
    const int above = m_channel.size() - pos - 1;
    if (above > m_config.tempCapacity) {
        log(t, "警告：货车 %d 让路需要 %d 个临时位，超过临时区容量 %d",
            id, above, m_config.tempCapacity);
        setError(err, errSize, "临时等候区容量不足，无法让路");
        return false;
    }

    for (int i = 0; i < above; ++i) {
        const int moved = m_channel.pop();
        m_trucks[moved].state = TruckState::Temp;
        m_temp.push(moved);
    }
    if (m_temp.size() > m_maxTemp) {
        m_maxTemp = m_temp.size();
    }

    m_channel.pop();  // 目标货车离开
    truck.state = TruckState::Left;
    truck.leaveTime = t;

    const double waited = t - truck.arriveTime;
    const double money = waited / 60.0 * m_config.feePerHour;
    m_revenue += money;
    ++m_served;
    m_totalWait += waited;

    log(t, "货车 %d（%s）离场，停留 %.1f 分钟，让路 %d 辆，费用 %.2f 元",
        id, cargoName(truck.type), waited, above, money);

    while (!m_temp.empty()) {
        const int back = m_temp.pop();
        m_trucks[back].state = TruckState::Channel;
        m_channel.push(back);
    }

    fillChannel(t);

    if (fee != nullptr) {
        *fee = money;
    }
    if (dwell != nullptr) {
        *dwell = waited;
    }
    return true;
}

bool TruckYard::hasTruck(int id) const {
    const Truck* t = findTruck(id);
    return t != nullptr && t->state != TruckState::Left;
}

const Truck* TruckYard::findTruck(int id) const {
    int index = -1;
    if (!m_index.get(id, index)) {
        return nullptr;
    }
    if (index < 0 || index >= m_trucks.size()) {
        return nullptr;
    }
    return &m_trucks[index];
}

bool TruckYard::checkInvariant() const {
    const int n = m_trucks.size();
    Vector<unsigned char> seen;
    seen.resize(n);
    for (int i = 0; i < n; ++i) {
        seen[i] = 0;
    }

    int active = 0;
    int left = 0;
    for (int i = 0; i < n; ++i) {
        const Truck& t = m_trucks[i];
        if (t.id <= 0 || !(t.arriveTime >= 0.0)) {
            return false;
        }
        if (t.state == TruckState::Left) {
            ++left;
        } else {
            ++active;
        }
        int mapped = -1;
        if (!m_index.get(t.id, mapped) || mapped != i) {
            return false;
        }
    }
    if (m_index.size() != n || m_arrived != active + left) {
        return false;
    }

    if (m_channel.size() > m_config.capacity || m_temp.size() > m_config.tempCapacity) {
        return false;
    }

    for (int i = 0; i < m_channel.size(); ++i) {
        const int index = m_channel.at(i);
        if (index < 0 || index >= n || seen[index] ||
            m_trucks[index].state != TruckState::Channel) {
            return false;
        }
        seen[index] = 1;
    }
    for (int i = 0; i < m_temp.size(); ++i) {
        const int index = m_temp.at(i);
        if (index < 0 || index >= n || seen[index] ||
            m_trucks[index].state != TruckState::Temp) {
            return false;
        }
        seen[index] = 1;
    }
    for (int k = 0; k < 3; ++k) {
        const Queue<int>& q = m_queues[k];
        for (int i = 0; i < q.size(); ++i) {
            const int index = q.at(i);
            if (index < 0 || index >= n || seen[index] ||
                m_trucks[index].state != TruckState::Waiting ||
                static_cast<int>(m_trucks[index].type) != k) {
                return false;
            }
            seen[index] = 1;
        }
    }
    for (int i = 0; i < n; ++i) {
        if (m_trucks[i].state != TruckState::Left && !seen[i]) {
            return false;
        }
    }
    return true;
}

YardStats TruckYard::stats() const {
    YardStats s;
    s.revenue = m_revenue;
    s.totalWait = m_totalWait;
    s.arrived = m_arrived;
    s.served = m_served;
    s.inChannel = m_channel.size();
    s.inTemp = m_temp.size();
    for (int i = 0; i < 3; ++i) {
        s.waiting[i] = m_queues[i].size();
    }
    s.freeSlots = m_config.capacity - m_channel.size();
    s.maxTempUsed = m_maxTemp;
    return s;
}

void TruckYard::fillChannel(double t) {
    while (m_channel.size() < m_config.capacity) {
        int best = -1;
        for (int i = 0; i < 3; ++i) {
            if (!m_queues[i].empty()) {
                best = i;
                break;
            }
        }
        if (best < 0) {
            break;
        }
        const int index = m_queues[best].dequeue();
        m_trucks[index].state = TruckState::Channel;
        m_trucks[index].enterTime = t;
        m_channel.push(index);
        log(t, "货车 %d 进入主通道（空位 %d）", m_trucks[index].id,
            m_config.capacity - m_channel.size());
    }
}

bool TruckYard::removeFromQueue(int index, CargoType type) {
    Queue<int>& q = m_queues[static_cast<int>(type)];
    for (int i = 0; i < q.size(); ++i) {
        if (q.at(i) == index) {
            q.removeAt(i);
            return true;
        }
    }
    return false;
}

int TruckYard::channelPos(int index) const {
    for (int i = 0; i < m_channel.size(); ++i) {
        if (m_channel.at(i) == index) {
            return i;
        }
    }
    return -1;
}

void TruckYard::log(double t, const char* fmt, ...) {
    Event e;
    e.time = t;
    va_list args;
    va_start(args, fmt);
    std::vsnprintf(e.text, sizeof(e.text), fmt, args);
    va_end(args);

    if (m_events.size() >= 300) {
        m_events.removeAt(0);
    }
    m_events.push_back(e);
    ++m_eventSeq;
}

void TruckYard::setError(char* err, int errSize, const char* msg) {
    if (err != nullptr && errSize > 0) {
        std::snprintf(err, static_cast<size_t>(errSize), "%s", msg);
    }
}

}  // namespace ds
