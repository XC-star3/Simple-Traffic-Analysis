#pragma once

#include "containers.h"

namespace ds {

// 邻接表的一条弧。无向边会存两条弧。
struct Arc {
    int to = -1;
    double dist = 0.0;
    double time = 0.0;
    bool active = false;
    int next = -1;
};

// 邻接表图：head[] + Arc[]。
class Graph {
public:
    Graph() = default;

    int addVertex() {
        m_head.push_back(-1);
        return m_head.size() - 1;
    }

    void addEdge(int u, int v, double dist, double time) {
        Arc a;
        a.to = v;
        a.dist = dist;
        a.time = time;
        a.active = true;
        a.next = m_head[u];
        m_arcs.push_back(a);
        m_head[u] = m_arcs.size() - 1;

        Arc b;
        b.to = u;
        b.dist = dist;
        b.time = time;
        b.active = true;
        b.next = m_head[v];
        m_arcs.push_back(b);
        m_head[v] = m_arcs.size() - 1;
    }

    // 阻断无向边：两条弧一起置灰。
    bool removeEdge(int u, int v) {
        bool found = false;
        for (int e = m_head[u]; e != -1; e = m_arcs[e].next) {
            if (m_arcs[e].to == v) {
                m_arcs[e].active = false;
                found = true;
            }
        }
        for (int e = m_head[v]; e != -1; e = m_arcs[e].next) {
            if (m_arcs[e].to == u) {
                m_arcs[e].active = false;
                found = true;
            }
        }
        return found;
    }

    bool restoreEdge(int u, int v) {
        bool found = false;
        for (int e = m_head[u]; e != -1; e = m_arcs[e].next) {
            if (m_arcs[e].to == v) {
                m_arcs[e].active = true;
                found = true;
            }
        }
        for (int e = m_head[v]; e != -1; e = m_arcs[e].next) {
            if (m_arcs[e].to == u) {
                m_arcs[e].active = true;
                found = true;
            }
        }
        return found;
    }

    bool hasEdge(int u, int v) const {
        for (int e = m_head[u]; e != -1; e = m_arcs[e].next) {
            if (m_arcs[e].to == v && m_arcs[e].active) {
                return true;
            }
        }
        return false;
    }

    void clear() {
        m_head.clear();
        m_arcs.clear();
    }

    int vertexCount() const { return m_head.size(); }
    int arcCount() const { return m_arcs.size(); }
    int head(int v) const { return m_head[v]; }
    const Arc& arc(int e) const { return m_arcs[e]; }
    const Vector<Arc>& arcs() const { return m_arcs; }

private:
    Vector<int> m_head;
    Vector<Arc> m_arcs;
};

}  // namespace ds
