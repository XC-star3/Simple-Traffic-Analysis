#pragma once

#include "containers.h"

namespace ds {

enum class CargoType { Fresh = 0, Normal = 1, Bulk = 2 };

inline const char* cargoName(CargoType t) {
    switch (t) {
        case CargoType::Fresh: return "生鲜件";
        case CargoType::Normal: return "普通件";
        case CargoType::Bulk: return "大件货";
    }
    return "未知";
}

enum class TruckState { Waiting = 0, Channel = 1, Temp = 2, Left = 3 };

struct Truck {
    int id = 0;
    CargoType type = CargoType::Normal;
    double arriveTime = 0.0;
    double enterTime = 0.0;
    double leaveTime = 0.0;
    TruckState state = TruckState::Left;
};

struct YardConfig {
    int capacity = 8;
    int tempCapacity = 7;
    double feePerHour = 5.0;
};

struct YardStats {
    double revenue = 0.0;
    double totalWait = 0.0;
    int arrived = 0;
    int served = 0;
    int inChannel = 0;
    int inTemp = 0;
    int waiting[3] = {0, 0, 0};
    int freeSlots = 0;
    int maxTempUsed = 0;
};

struct Event {
    double time = 0.0;
    char text[200] = {0};
};

// 基础题 2：快递中转栈-队列混合调度模拟。
class TruckYard {
public:
    TruckYard() = default;

    bool configure(int capacity, int tempCapacity, double feePerHour);
    bool configured() const { return m_configured; }

    int capacity() const { return m_config.capacity; }
    int tempCapacity() const { return m_config.tempCapacity; }
    double feePerHour() const { return m_config.feePerHour; }

    bool arrive(int id, CargoType type, double t, char* err = nullptr, int errSize = 0);
    bool depart(int id, double t, double* fee = nullptr, double* dwell = nullptr,
                char* err = nullptr, int errSize = 0);

    bool hasTruck(int id) const;
    const Truck* findTruck(int id) const;

    const Stack<int>& channel() const { return m_channel; }
    const Stack<int>& temp() const { return m_temp; }
    const Queue<int>& queue(CargoType type) const { return m_queues[static_cast<int>(type)]; }
    const Vector<Truck>& trucks() const { return m_trucks; }
    const Truck& truckAt(int index) const { return m_trucks[index]; }
    const Vector<Event>& events() const { return m_events; }
    long long eventSeq() const { return m_eventSeq; }

    YardStats stats() const;
    void resetEvents() { m_events.clear(); }

    // 供测试使用的状态一致性检查：
    // 到达数 = 在场数 + 离场数；通道/临时区/队列中的下标有效且不重复。
    bool checkInvariant() const;

private:
    void fillChannel(double t);
    void log(double t, const char* fmt, ...);
    bool removeFromQueue(int index, CargoType type);
    int channelPos(int index) const;
    static void setError(char* err, int errSize, const char* msg);

    YardConfig m_config;
    bool m_configured = false;

    Vector<Truck> m_trucks;
    HashMap<int> m_index;  // 货车编号 -> m_trucks 下标
    Stack<int> m_channel;  // 栈底为通道最里面，栈顶为入口
    Stack<int> m_temp;
    Queue<int> m_queues[3];

    Vector<Event> m_events;
    long long m_eventSeq = 0;
    double m_revenue = 0.0;
    int m_arrived = 0;
    int m_served = 0;
    int m_maxTemp = 0;
    double m_totalWait = 0.0;
};

}  // namespace ds
