#pragma once

#include "graph.h"

namespace ds {

enum class WeightMode { Distance = 0, Time = 1 };

struct HeapNode {
    double key = 0.0;
    int node = -1;

    bool operator<(const HeapNode& other) const {
        if (key != other.key) {
            return key < other.key;
        }
        return node < other.node;
    }
};

struct PathResult {
    bool reachable = false;
    double total = 0.0;
    Vector<int> nodes;
};

constexpr double kInfinity = 1e300;

// 单源最短路：Dijkstra + 自研二叉堆。
void dijkstra(const Graph& g, int src, WeightMode mode,
              Vector<double>& dist, Vector<int>& prev);

// 两点最短路，返回路径（含起点、终点）与总权重。
PathResult shortestPath(const Graph& g, int src, int dst, WeightMode mode);

}  // namespace ds
