#pragma once

#include "edgar/graphs/undirected_graph.hpp"

#include <stdexcept>
#include <vector>

namespace edgar::graphs {

/// Planar faces of an undirected graph (combinatorial embedding via Boost Graph Library).
/// Vertex labels must be `0 .. n-1` (each vertex id equals its compact index). Other labelings are not supported yet.
/// \throws std::invalid_argument if the graph is not planar or not connected (when non-empty).
std::vector<std::vector<int>> get_planar_faces(const UndirectedAdjacencyListGraph<int>& g);

} // namespace edgar::graphs
