#include "ds/bike_system.h"
#include "ds/containers.h"
#include "ds/truck_yard.h"

#include <cmath>
#include <cstdio>

namespace {

int g_failures = 0;

#define CHECK(cond)                                                          \
    do {                                                                     \
        if (!(cond)) {                                                       \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);      \
            ++g_failures;                                                    \
        }                                                                    \
    } while (0)

void testContainers() {
    ds::Vector<int> v;
    for (int i = 0; i < 100; ++i) {
        v.push_back(i);
    }
    CHECK(v.size() == 100);
    CHECK(v[0] == 0 && v[99] == 99);
    ds::Vector<int> copy = v;
    CHECK(copy.size() == 100 && copy[50] == 50);
    v.removeAt(10);
    CHECK(v.size() == 99 && v[10] == 11);
    v.reverse();
    CHECK(v[0] == 99 && v[98] == 0);
    v.clear();
    CHECK(v.empty());

    ds::Stack<int> st;
    st.push(1);
    st.push(2);
    CHECK(st.top() == 2);
    CHECK(st.pop() == 2);
    CHECK(st.pop() == 1);

    ds::Queue<int> q;
    for (int i = 0; i < 20; ++i) {
        q.enqueue(i);
    }
    CHECK(q.size() == 20 && q.front() == 0);
    q.removeAt(5);
    CHECK(q.size() == 19 && q.at(5) == 6);
    CHECK(q.dequeue() == 0);

    ds::MinHeap<int> heap;
    const int values[] = {5, 1, 9, 3, 7, 2, 8};
    for (int x : values) {
        heap.push(x);
    }
    int last = -1;
    while (!heap.empty()) {
        const int x = heap.pop();
        CHECK(x >= last);
        last = x;
    }

    ds::HashMap<int> map;
    for (int i = 1; i <= 500; ++i) {
        map.put(i, i * 3);
    }
    CHECK(map.size() == 500);
    for (int i = 1; i <= 500; ++i) {
        int value = 0;
        CHECK(map.get(i, value) && value == i * 3);
    }
    CHECK(map.remove(250));
    CHECK(!map.contains(250));
    int tmp = 0;
    CHECK(!map.get(250, tmp));
    CHECK(map.size() == 499);
}

void testTruckYard() {
    ds::TruckYard yard;
    CHECK(yard.configure(3, 2, 60.0));
    CHECK(yard.arrive(101, ds::CargoType::Fresh, 0.0));
    CHECK(yard.arrive(102, ds::CargoType::Normal, 1.0));
    CHECK(yard.arrive(103, ds::CargoType::Bulk, 2.0));
    CHECK(yard.channel().size() == 3);
    CHECK(yard.arrive(104, ds::CargoType::Fresh, 3.0));
    CHECK(yard.channel().size() == 3);
    CHECK(yard.queue(ds::CargoType::Fresh).size() == 1);

    double fee = -1.0;
    double dwell = -1.0;
    CHECK(yard.depart(101, 4.0, &fee, &dwell));
    CHECK(std::fabs(fee - 4.0) < 1e-9);
    CHECK(std::fabs(dwell - 4.0) < 1e-9);
    CHECK(yard.channel().size() == 3);
    CHECK(yard.channel().at(0) == 1);  // 102 在栈底
    CHECK(yard.channel().at(2) == 3);  // 104 补位到栈顶

    // 取消排队：不计费。
    ds::TruckYard cancelYard;
    CHECK(cancelYard.configure(1, 0, 10.0));
    CHECK(cancelYard.arrive(201, ds::CargoType::Bulk, 0.0));
    CHECK(cancelYard.arrive(202, ds::CargoType::Fresh, 1.0));
    CHECK(cancelYard.depart(202, 2.0, &fee, &dwell));
    CHECK(std::fabs(fee) < 1e-9);

    // 临时区不足：拒绝离场且状态不变。
    ds::TruckYard small;
    CHECK(small.configure(3, 1, 10.0));
    CHECK(small.arrive(301, ds::CargoType::Fresh, 0.0));
    CHECK(small.arrive(302, ds::CargoType::Normal, 0.0));
    CHECK(small.arrive(303, ds::CargoType::Bulk, 0.0));
    char err[128] = {0};
    CHECK(!small.depart(301, 1.0, nullptr, nullptr, err, sizeof(err)));
    CHECK(err[0] != '\0');
    CHECK(small.channel().size() == 3);

    // 优先级：空位优先给生鲜。
    ds::TruckYard priority;
    CHECK(priority.configure(2, 0, 10.0));
    CHECK(priority.arrive(401, ds::CargoType::Bulk, 0.0));
    CHECK(priority.arrive(402, ds::CargoType::Normal, 0.0));
    CHECK(priority.arrive(403, ds::CargoType::Fresh, 0.0));
    CHECK(priority.arrive(404, ds::CargoType::Bulk, 0.0));
    CHECK(priority.depart(402, 1.0));  // 栈顶离开，空出一个位置
    CHECK(priority.channel().at(0) == 0 && priority.truckAt(0).id == 401);
    CHECK(priority.channel().at(1) == 2 && priority.truckAt(2).id == 403);  // 生鲜优先
    CHECK(priority.queue(ds::CargoType::Bulk).size() == 1);

    CHECK(!priority.arrive(401, ds::CargoType::Fresh, 2.0));  // 编号重复
    const ds::YardStats stats = priority.stats();
    CHECK(stats.arrived == 4);
    CHECK(stats.served == 1);
    CHECK(stats.inChannel == 2);
    CHECK(stats.freeSlots == 0);
}

void testDijkstraAgainstFloyd() {
    const int n = 6;
    const double inf = 1e300;
    double f[6][6];
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            f[i][j] = (i == j) ? 0.0 : inf;
        }
    }
    ds::Graph g;
    for (int i = 0; i < n; ++i) {
        g.addVertex();
    }
    const int edges[][3] = {
        {0, 1, 2}, {0, 2, 5}, {1, 2, 1}, {1, 3, 4},
        {2, 3, 2}, {3, 4, 3}, {2, 4, 7}, {4, 5, 1},
    };
    for (const auto& e : edges) {
        g.addEdge(e[0], e[1], e[2], e[2]);
        f[e[0]][e[1]] = f[e[1]][e[0]] = e[2];
    }
    for (int k = 0; k < n; ++k) {
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n; ++j) {
                if (f[i][k] + f[k][j] < f[i][j]) {
                    f[i][j] = f[i][k] + f[k][j];
                }
            }
        }
    }
    for (int src = 0; src < n; ++src) {
        ds::Vector<double> dist;
        ds::Vector<int> prev;
        ds::dijkstra(g, src, ds::WeightMode::Distance, dist, prev);
        for (int dst = 0; dst < n; ++dst) {
            CHECK(std::fabs(dist[dst] - f[src][dst]) < 1e-9);
        }
    }
}

void buildBikeNetwork(ds::BikeSystem& s) {
    CHECK(s.addStation(1, "A", 10, 20, 0, 0));
    CHECK(s.addStation(2, "B", 2, 20, 200, 0));
    CHECK(s.addStation(3, "C", 18, 20, 400, 0));
    CHECK(s.addStation(4, "D", 5, 20, 600, 0));
    CHECK(s.addStation(5, "E", 6, 10, 100, 300));
    CHECK(s.addRoad(1, 2, 10.0, 5.0));
    CHECK(s.addRoad(2, 3, 10.0, 5.0));
    CHECK(s.addRoad(3, 4, 10.0, 5.0));
    CHECK(s.addRoad(1, 5, 5.0, 3.0));
}

void testBikeSystem() {
    ds::BikeSystem s;
    buildBikeNetwork(s);
    CHECK(s.stationCount() == 5);
    CHECK(s.addRoad(1, 2, 9.0, 4.0) == false);  // 重复边

    ds::Vector<int> path;
    double total = 0.0;
    CHECK(s.shortest(1, 4, ds::WeightMode::Distance, path, total));
    CHECK(std::fabs(total - 30.0) < 1e-9);
    CHECK(path.size() == 4);
    CHECK(path[0] == 1 && path[3] == 4);

    CHECK(s.blockRoad(2, 3));
    CHECK(!s.hasActiveRoad(2, 3));
    CHECK(!s.shortest(1, 4, ds::WeightMode::Distance, path, total));
    CHECK(!s.connected(1, 4));
    CHECK(s.restoreRoad(2, 3));
    CHECK(s.shortest(1, 4, ds::WeightMode::Distance, path, total));

    ds::Vector<int> targets;
    targets.push_back(3);
    targets.push_back(4);
    ds::Vector<int> tour;
    ds::Vector<int> full;
    double planTotal = 0.0;
    char err[128] = {0};
    CHECK(s.planRoute(1, targets, ds::WeightMode::Distance, tour, full, planTotal, err,
                      sizeof(err)));
    CHECK(tour.size() == 4);  // 1 -> 3 -> 4 -> 1
    CHECK(tour.front() == 1 && tour.back() == 1);
    CHECK(full.front() == 1 && full.back() == 1);
    CHECK(planTotal > 0.0);

    ds::DispatchReport report;
    CHECK(s.simulate(tour, 8, 20, 80, report));
    CHECK(report.ok);
    CHECK(report.stops.size() == tour.size());
    CHECK(report.totalDist > 0.0);
    CHECK(report.totalTime > 0.0);

    // 断边后模拟应失败（路径不可达）。
    CHECK(s.blockRoad(2, 3));
    CHECK(!s.simulate(tour, 8, 20, 80, report));
}

void testBikeSingleTarget() {
    ds::BikeSystem s;
    buildBikeNetwork(s);
    ds::Vector<int> targets;
    targets.push_back(4);
    ds::Vector<int> tour;
    ds::Vector<int> full;
    double total = 0.0;
    char err[128] = {0};
    CHECK(s.planRoute(1, targets, ds::WeightMode::Distance, tour, full, total, err,
                      sizeof(err)));
    CHECK(tour.size() == 3);  // 1 -> 4 -> 1
    CHECK(tour[0] == 1 && tour[1] == 4 && tour[2] == 1);
    CHECK(full.front() == 1 && full.back() == 1);
    CHECK(std::fabs(total - 60.0) < 1e-9);

    ds::Vector<int> path;
    double one = -1.0;
    CHECK(s.shortest(1, 1, ds::WeightMode::Distance, path, one));
    CHECK(path.size() == 1 && path[0] == 1 && std::fabs(one) < 1e-9);

    CHECK(!s.addStation(1, "重复", 1, 10, 0, 0));
    CHECK(s.blockRoad(2, 3));
    CHECK(!s.blockRoad(2, 3));  // 已阻断
}

}  // namespace

int main() {
    testContainers();
    testTruckYard();
    testDijkstraAgainstFloyd();
    testBikeSystem();
    testBikeSingleTarget();
    if (g_failures == 0) {
        std::printf("ALL TESTS PASSED\n");
        return 0;
    }
    std::printf("%d TEST(S) FAILED\n", g_failures);
    return 1;
}
