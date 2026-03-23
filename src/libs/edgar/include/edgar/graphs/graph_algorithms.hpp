#pragma once

#include "edgar/graphs/undirected_graph.hpp"

#include <queue>
#include <set>

namespace edgar::graphs {

template <typename T>
bool is_connected(const UndirectedAdjacencyListGraph<T>& g) {
    if (g.vertex_count() == 0) {
        return true;
    }
    const auto verts = g.vertices();
    std::set<T> seen;
    std::queue<T> q;
    q.push(verts.front());
    seen.insert(verts.front());
    while (!q.empty()) {
        const T v = q.front();
        q.pop();
        for (const T& n : g.neighbours(v)) {
            if (!seen.contains(n)) {
                seen.insert(n);
                q.push(n);
            }
        }
    }
    return seen.size() == g.vertex_count();
}

/// True if `g` is a connected tree (|E| = |V| − 1).
template <typename T>
bool is_tree(const UndirectedAdjacencyListGraph<T>& g) {
    const std::size_t n = g.vertex_count();
    if (n == 0) {
        return true;
    }
    if (!is_connected(g)) {
        return false;
    }
    std::size_t edge_count = 0;
    for (const T& v : g.vertices()) {
        edge_count += g.neighbours(v).size();
    }
    edge_count /= 2;
    return edge_count + 1 == n;
}

} // namespace edgar::graphs
