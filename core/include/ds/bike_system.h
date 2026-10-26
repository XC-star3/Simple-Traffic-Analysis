#pragma once

#include "dijkstra.h"
#include "graph.h"

namespace ds {

struct Station {
    int id = 0;
    char name[64] = {0};
    int bikes = 0;
    int capacity = 0;
    double x = 0.0;
    double y = 0.0;
};

struct Road {
    int u = 0;
    int v = 0;
    double dist = 0.0;
    double time = 0.0;
};

struct Alert {
    int stationId = 0;
    char text[160] = {0};
};

struct StopRecord {
    int stationId = 0;
    int before = 0;
    int after = 0;
    int unloaded = 0;
    int loaded = 0;
    int load = 0;
    bool alert = false;
};

struct DispatchReport {
    bool ok = false;
    char message[200] = {0};
    Vector<StopRecord> stops;
    double totalDist = 0.0;
    double totalTime = 0.0;
    int alertCount = 0;
};

// 扩展题 7：城市公共自行车调度路径规划系统。
class BikeSystem {
public:
    bool addStation(int id, const char* name, int bikes, int capacity,
                    double x, double y, char* err = nullptr, int errSize = 0);
    bool addRoad(int u, int v, double dist, double time,
                 char* err = nullptr, int errSize = 0);

    bool blockRoad(int u, int v);
    bool restoreRoad(int u, int v);
    bool hasActiveRoad(int u, int v) const;
    bool connected(int a, int b) const;

    int stationCount() const { return m_stations.size(); }
    const Station& stationAt(int i) const { return m_stations[i]; }
    const Station* findStation(int id) const;
    int indexOf(int id) const;
    const Vector<Station>& stations() const { return m_stations; }
    const Vector<Road>& roads() const { return m_roads; }
    const Graph& graph() const { return m_graph; }
    const Vector<Alert>& alerts() const { return m_alerts; }
    void clearAlerts() { m_alerts.clear(); }

    bool shortest(int src, int dst, WeightMode mode,
                  Vector<int>& path, double& total) const;

    // 规划：从总站出发访问目标集合，最后回到总站。
    // tour 为站点编号序列（含首尾总站），fullPath 为拼接后的完整途经站点。
    bool planRoute(int depot, const Vector<int>& targets, WeightMode mode,
                   Vector<int>& tour, Vector<int>& fullPath, double& total,
                   char* err = nullptr, int errSize = 0) const;

    // 模拟调度：按 tour 顺序装卸自行车，返回每站记录与告警。
    bool simulate(const Vector<int>& tour, int truckCapacity,
                  int lowPercent, int highPercent, DispatchReport& report);

    void clear();

private:
    void addAlert(int stationId, const char* fmt, ...);
    static void setError(char* err, int errSize, const char* msg);

    Vector<Station> m_stations;
    Vector<Road> m_roads;
    Graph m_graph;
    Vector<Alert> m_alerts;
};

}  // namespace ds
