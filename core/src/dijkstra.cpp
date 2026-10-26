#include "ds/dijkstra.h"

namespace ds {

void dijkstra(const Graph& g, int src, WeightMode mode,
              Vector<double>& dist, Vector<int>& prev) {
    const int n = g.vertexCount();
    dist.clear();
    dist.resize(n);
    prev.clear();
    prev.resize(n);
    for (int i = 0; i < n; ++i) {
        dist[i] = kInfinity;
        prev[i] = -1;
    }
    if (src < 0 || src >= n) {
        return;
    }

    Vector<unsigned char> done;
    done.resize(n);
    for (int i = 0; i < n; ++i) {
        done[i] = 0;
    }

    MinHeap<HeapNode> heap;
    dist[src] = 0.0;
    HeapNode start;
    start.key = 0.0;
    start.node = src;
    heap.push(start);

    while (!heap.empty()) {
        HeapNode cur = heap.pop();
        const int u = cur.node;
        if (u < 0 || u >= n || done[u]) {
            continue;
        }
        if (cur.key > dist[u]) {
            continue;  // 懒删除：堆里的旧条目
        }
        done[u] = 1;

        for (int e = g.head(u); e != -1; e = g.arc(e).next) {
            const Arc& a = g.arc(e);
            if (!a.active) {
                continue;
            }
            const double w = (mode == WeightMode::Distance) ? a.dist : a.time;
            if (w < 0.0) {
                continue;
            }
            const double nd = dist[u] + w;
            if (nd < dist[a.to]) {
                dist[a.to] = nd;
                prev[a.to] = u;
                HeapNode next;
                next.key = nd;
                next.node = a.to;
                heap.push(next);
            }
        }
    }
}

PathResult shortestPath(const Graph& g, int src, int dst, WeightMode mode) {
    PathResult result;
    const int n = g.vertexCount();
    if (src < 0 || src >= n || dst < 0 || dst >= n) {
        return result;
    }
    if (src == dst) {
        result.reachable = true;
        result.total = 0.0;
        result.nodes.push_back(src);
        return result;
    }

    Vector<double> dist;
    Vector<int> prev;
    dijkstra(g, src, mode, dist, prev);
    if (dist[dst] >= kInfinity / 2.0) {
        return result;
    }

    Vector<int> path;
    for (int v = dst; v != -1; v = prev[v]) {
        path.push_back(v);
        if (v == src) {
            break;
        }
    }
    if (path.empty() || path.back() != src) {
        return result;
    }
    path.reverse();
    result.reachable = true;
    result.total = dist[dst];
    result.nodes = path;
    return result;
}

}  // namespace ds
