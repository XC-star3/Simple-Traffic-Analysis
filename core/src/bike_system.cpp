#include "ds/bike_system.h"

#include <cmath>
#include <cstdarg>
#include <cstdio>

namespace ds {
namespace {

void copyName(char* dst, size_t n, const char* src) {
    if (dst == nullptr || n == 0) {
        return;
    }
    if (src == nullptr) {
        dst[0] = '\0';
        return;
    }
    std::snprintf(dst, n, "%s", src);
}

bool validCoord(double v) {
    return v == v && v > -1e9 && v < 1e9;
}

}  // namespace

bool BikeSystem::addStation(int id, const char* name, int bikes, int capacity,
                            double x, double y, char* err, int errSize) {
    if (id <= 0) {
        setError(err, errSize, "站点编号必须为正整数");
        return false;
    }
    if (findStation(id) != nullptr) {
        setError(err, errSize, "站点编号重复");
        return false;
    }
    if (name == nullptr || name[0] == '\0') {
        setError(err, errSize, "站点名称不能为空");
        return false;
    }
    if (capacity <= 0 || bikes < 0 || bikes > capacity) {
        setError(err, errSize, "站点容量/车辆数非法");
        return false;
    }
    if (!validCoord(x) || !validCoord(y)) {
        setError(err, errSize, "站点坐标非法");
        return false;
    }

    Station s;
    s.id = id;
    copyName(s.name, sizeof(s.name), name);
    s.bikes = bikes;
    s.capacity = capacity;
    s.x = x;
    s.y = y;
    m_stations.push_back(s);
    m_stationIndex.put(id, m_stations.size() - 1);
    m_graph.addVertex();
    return true;
}

bool BikeSystem::addRoad(int u, int v, double dist, double time,
                         char* err, int errSize) {
    if (u == v) {
        setError(err, errSize, "不允许自环道路");
        return false;
    }
    const int iu = indexOf(u);
    const int iv = indexOf(v);
    if (iu < 0 || iv < 0) {
        setError(err, errSize, "道路端点不存在");
        return false;
    }
    if (!(dist > 0.0) || !(time > 0.0) || !std::isfinite(dist) || !std::isfinite(time)) {
        setError(err, errSize, "距离和耗时必须为正数");
        return false;
    }
    for (int i = 0; i < m_roads.size(); ++i) {
        const Road& r = m_roads[i];
        if ((r.u == u && r.v == v) || (r.u == v && r.v == u)) {
            setError(err, errSize, "该道路已存在");
            return false;
        }
    }

    m_graph.addEdge(iu, iv, dist, time);
    Road r;
    r.u = u;
    r.v = v;
    r.dist = dist;
    r.time = time;
    m_roads.push_back(r);
    return true;
}

bool BikeSystem::blockRoad(int u, int v) {
    if (!hasActiveRoad(u, v)) {
        return false;
    }
    return m_graph.removeEdge(indexOf(u), indexOf(v));
}

bool BikeSystem::restoreRoad(int u, int v) {
    const int iu = indexOf(u);
    const int iv = indexOf(v);
    if (iu < 0 || iv < 0) {
        return false;
    }
    return m_graph.restoreEdge(iu, iv);
}

bool BikeSystem::hasActiveRoad(int u, int v) const {
    const int iu = indexOf(u);
    const int iv = indexOf(v);
    if (iu < 0 || iv < 0) {
        return false;
    }
    return m_graph.hasEdge(iu, iv);
}

bool BikeSystem::connected(int a, int b) const {
    const int s = indexOf(a);
    const int t = indexOf(b);
    if (s < 0 || t < 0) {
        return false;
    }
    if (s == t) {
        return true;
    }

    Vector<unsigned char> visited;
    visited.resize(m_graph.vertexCount());
    for (int i = 0; i < visited.size(); ++i) {
        visited[i] = 0;
    }
    Queue<int> q;
    q.enqueue(s);
    visited[s] = 1;
    while (!q.empty()) {
        const int u = q.dequeue();
        if (u == t) {
            return true;
        }
        for (int e = m_graph.head(u); e != -1; e = m_graph.arc(e).next) {
            const Arc& arc = m_graph.arc(e);
            if (!arc.active || visited[arc.to]) {
                continue;
            }
            visited[arc.to] = 1;
            q.enqueue(arc.to);
        }
    }
    return false;
}

const Station* BikeSystem::findStation(int id) const {
    const int index = indexOf(id);
    if (index < 0 || index >= m_stations.size()) {
        return nullptr;
    }
    return &m_stations[index];
}

int BikeSystem::indexOf(int id) const {
    int index = -1;
    if (!m_stationIndex.get(id, index)) {
        return -1;
    }
    if (index < 0 || index >= m_stations.size() || m_stations[index].id != id) {
        return -1;
    }
    return index;
}

bool BikeSystem::shortest(int src, int dst, WeightMode mode,
                          Vector<int>& path, double& total) const {
    path.clear();
    total = 0.0;
    const int s = indexOf(src);
    const int t = indexOf(dst);
    if (s < 0 || t < 0) {
        return false;
    }
    const PathResult r = shortestPath(m_graph, s, t, mode);
    if (!r.reachable) {
        return false;
    }
    // 对外统一返回站点编号，而不是图顶点下标。
    for (int i = 0; i < r.nodes.size(); ++i) {
        path.push_back(m_stations[r.nodes[i]].id);
    }
    total = r.total;
    return true;
}

bool BikeSystem::planRoute(int depot, const Vector<int>& targets, WeightMode mode,
                           Vector<int>& tour, Vector<int>& fullPath, double& total,
                           char* err, int errSize) const {
    tour.clear();
    fullPath.clear();
    total = 0.0;

    if (indexOf(depot) < 0) {
        setError(err, errSize, "调度总站不存在");
        return false;
    }

    Vector<int> ids;
    ids.push_back(depot);
    for (int i = 0; i < targets.size(); ++i) {
        const int id = targets[i];
        if (id == depot) {
            continue;
        }
        if (indexOf(id) < 0) {
            setError(err, errSize, "目标站点不存在");
            return false;
        }
        bool dup = false;
        for (int k = 0; k < ids.size(); ++k) {
            if (ids[k] == id) {
                dup = true;
                break;
            }
        }
        if (!dup) {
            ids.push_back(id);
        }
    }
    if (ids.size() <= 1) {
        setError(err, errSize, "请至少选择一个目标站点");
        return false;
    }

    const int count = ids.size();
    Vector<Vector<double>> distRows;
    Vector<Vector<int>> prevRows;
    for (int i = 0; i < count; ++i) {
        Vector<double> d;
        Vector<int> p;
        dijkstra(m_graph, indexOf(ids[i]), mode, d, p);
        distRows.push_back(d);
        prevRows.push_back(p);
    }
    for (int j = 1; j < count; ++j) {
        if (distRows[0][indexOf(ids[j])] >= kInfinity / 2.0) {
            setError(err, errSize, "存在不可达的目标站点");
            return false;
        }
    }

    // 最近邻构造初始顺序（order 存 ids 的下标）。
    Vector<int> order;
    order.push_back(0);
    Vector<unsigned char> used;
    used.resize(count);
    for (int i = 0; i < count; ++i) {
        used[i] = 0;
    }
    used[0] = 1;
    int current = 0;
    for (int step = 1; step < count; ++step) {
        int best = -1;
        double bestD = kInfinity;
        for (int j = 1; j < count; ++j) {
            if (used[j]) {
                continue;
            }
            const double d = distRows[current][indexOf(ids[j])];
            if (d < bestD) {
                bestD = d;
                best = j;
            }
        }
        if (best < 0) {
            setError(err, errSize, "目标站点不可达");
            return false;
        }
        order.push_back(best);
        used[best] = 1;
        current = best;
    }
    order.push_back(0);

    // 2-opt 改进（启发式，不保证全局最优）。
    for (int pass = 0; pass < 20; ++pass) {
        bool improved = false;
        for (int i = 1; i + 2 < order.size(); ++i) {
            for (int j = i + 1; j + 1 < order.size(); ++j) {
                const int a = order[i - 1];
                const int b = order[i];
                const int c = order[j];
                const int e = order[j + 1];
                const double oldCost =
                    distRows[a][indexOf(ids[b])] + distRows[c][indexOf(ids[e])];
                const double newCost =
                    distRows[a][indexOf(ids[c])] + distRows[b][indexOf(ids[e])];
                if (newCost + 1e-7 < oldCost) {
                    for (int l = i, r = j; l < r; ++l, --r) {
                        const int tmp = order[l];
                        order[l] = order[r];
                        order[r] = tmp;
                    }
                    improved = true;
                }
            }
        }
        if (!improved) {
            break;
        }
    }

    for (int i = 0; i < order.size(); ++i) {
        tour.push_back(ids[order[i]]);
    }

    for (int k = 0; k + 1 < order.size(); ++k) {
        const int fromPos = order[k];
        const int toPos = order[k + 1];
        const int srcV = indexOf(ids[fromPos]);
        const int dstV = indexOf(ids[toPos]);
        const double leg = distRows[fromPos][dstV];
        if (leg >= kInfinity / 2.0) {
            setError(err, errSize, "规划途中出现不可达路段");
            return false;
        }

        const Vector<int>& pv = prevRows[fromPos];
        Vector<int> seg;
        int v = dstV;
        int guard = 0;
        while (v != -1 && guard++ < 100000) {
            seg.push_back(v);
            if (v == srcV) {
                break;
            }
            v = pv[v];
        }
        if (seg.empty() || seg.back() != srcV) {
            setError(err, errSize, "路径回溯失败");
            return false;
        }
        seg.reverse();
        for (int q = 0; q < seg.size(); ++q) {
            const int sid = m_stations[seg[q]].id;
            if (fullPath.empty() || fullPath[fullPath.size() - 1] != sid) {
                fullPath.push_back(sid);
            }
        }
        total += leg;
    }
    return true;
}

bool BikeSystem::simulate(const Vector<int>& tour, int truckCapacity,
                          int lowPercent, int highPercent, DispatchReport& report) {
    report = DispatchReport();

    if (tour.size() < 2) {
        std::snprintf(report.message, sizeof(report.message), "请先规划调度路线");
        return false;
    }
    if (truckCapacity <= 0) {
        std::snprintf(report.message, sizeof(report.message), "调度车载量必须为正数");
        return false;
    }
    if (lowPercent < 0 || highPercent > 100 || lowPercent > highPercent) {
        std::snprintf(report.message, sizeof(report.message), "告警阈值非法");
        return false;
    }
    for (int k = 0; k < tour.size(); ++k) {
        if (indexOf(tour[k]) < 0) {
            std::snprintf(report.message, sizeof(report.message), "路线中存在未知站点");
            return false;
        }
    }

    // 先验证并计算所有路段，再修改站点库存；失败时保持数据不变。
    Vector<double> legDist;
    Vector<double> legTime;
    for (int k = 0; k + 1 < tour.size(); ++k) {
        Vector<int> p;
        double d = 0.0;
        if (!shortest(tour[k], tour[k + 1], WeightMode::Distance, p, d)) {
            std::snprintf(report.message, sizeof(report.message), "路段不可达，无法完成调度");
            return false;
        }
        legDist.push_back(d);
        if (!shortest(tour[k], tour[k + 1], WeightMode::Time, p, d)) {
            std::snprintf(report.message, sizeof(report.message), "路段不可达，无法完成调度");
            return false;
        }
        legTime.push_back(d);
    }

    m_alerts.clear();

    int load = truckCapacity / 2;
    const int last = tour.size() - 1;
    for (int k = 0; k < tour.size(); ++k) {
        const int index = indexOf(tour[k]);
        Station& st = m_stations[index];
        StopRecord rec;
        rec.stationId = st.id;
        rec.before = st.bikes;
        rec.unloaded = 0;
        rec.loaded = 0;

        const int low = st.capacity * lowPercent / 100;
        const int high = st.capacity * highPercent / 100;

        if (k == 0) {
            int take = truckCapacity / 2;
            if (take > st.bikes) {
                take = st.bikes;
            }
            st.bikes -= take;
            load = take;
            rec.loaded = take;
        } else if (k == last) {
            int room = st.capacity - st.bikes;
            int drop = load;
            if (drop > room) {
                drop = room;
            }
            if (drop > 0) {
                st.bikes += drop;
                load -= drop;
                rec.unloaded = drop;
            }
            if (load > 0) {
                rec.alert = true;
                addAlert(st.id, "总站容量不足，调度车剩余 %d 辆未卸下", load);
            }
        } else {
            const int need = low - st.bikes;
            if (need > 0) {
                const int drop = need < load ? need : load;
                st.bikes += drop;
                load -= drop;
                rec.unloaded = drop;
            }
            const int surplus = st.bikes - high;
            if (surplus > 0) {
                const int room = truckCapacity - load;
                const int take = surplus < room ? surplus : room;
                st.bikes -= take;
                load += take;
                rec.loaded = take;
            }
        }

        rec.after = st.bikes;
        rec.load = load;
        if (st.bikes > st.capacity) {
            rec.alert = true;
            addAlert(st.id, "站点超出最大容量：%d/%d", st.bikes, st.capacity);
        }
        if (k > 0 && k < last) {
            if (st.bikes < low) {
                rec.alert = true;
                addAlert(st.id, "站点低于下限：%d/%d", st.bikes, low);
            } else if (st.bikes > high) {
                rec.alert = true;
                addAlert(st.id, "站点高于上限：%d/%d", st.bikes, high);
            }
        }
        report.stops.push_back(rec);
    }

    for (int i = 0; i < legDist.size(); ++i) {
        report.totalDist += legDist[i];
        report.totalTime += legTime[i];
    }

    report.alertCount = m_alerts.size();
    report.ok = true;
    std::snprintf(report.message, sizeof(report.message),
                  "调度完成：%d 站，总里程 %.2f km，总耗时 %.1f 分钟，告警 %d 条",
                  static_cast<int>(tour.size()), report.totalDist / 1000.0,
                  report.totalTime, report.alertCount);
    return true;
}

void BikeSystem::addAlert(int stationId, const char* fmt, ...) {
    Alert a;
    a.stationId = stationId;
    va_list args;
    va_start(args, fmt);
    std::vsnprintf(a.text, sizeof(a.text), fmt, args);
    va_end(args);
    if (m_alerts.size() >= 200) {
        m_alerts.removeAt(0);
    }
    m_alerts.push_back(a);
}

void BikeSystem::clear() {
    m_stations.clear();
    m_roads.clear();
    m_graph.clear();
    m_stationIndex.clear();
    m_alerts.clear();
}

void BikeSystem::setError(char* err, int errSize, const char* msg) {
    if (err != nullptr && errSize > 0) {
        std::snprintf(err, static_cast<size_t>(errSize), "%s", msg);
    }
}

}  // namespace ds
