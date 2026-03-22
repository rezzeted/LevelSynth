#include "edgar/graphs/planar_faces.hpp"

#include "edgar/graphs/graph_algorithms.hpp"

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/boyer_myrvold_planar_test.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/planar_face_traversal.hpp>

#include <algorithm>
#include <map>
#include <set>
#include <stdexcept>
#include <utility>

namespace edgar::graphs {

namespace {

using BoostGraph = boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS,
                                         boost::property<boost::vertex_index_t, int>,
                                         boost::property<boost::edge_index_t, int>>;

struct FaceCollector : public boost::planar_face_traversal_visitor {
    std::vector<std::vector<int>> faces;
    std::vector<int> current;

    void begin_face() { current.clear(); }

    template <typename Vertex>
    void next_vertex(Vertex v) {
        current.push_back(static_cast<int>(v));
    }

    void end_face() {
        if (!current.empty()) {
            faces.push_back(current);
        }
    }
};

} // namespace

std::vector<std::vector<int>> get_planar_faces(const UndirectedAdjacencyListGraph<int>& g) {
    if (g.vertex_count() == 0) {
        return {};
    }
    if (!is_connected(g)) {
        throw std::invalid_argument("get_planar_faces: graph must be connected");
    }

    const auto verts = g.vertices();
    std::map<int, int> label_to_index;
    for (std::size_t i = 0; i < verts.size(); ++i) {
        label_to_index.emplace(verts[i], static_cast<int>(i));
    }

    BoostGraph bg(static_cast<int>(verts.size()));
    std::set<std::pair<int, int>> seen_edges;
    for (const int& v : verts) {
        const int vi = label_to_index.at(v);
        for (const int& n : g.neighbours(v)) {
            const int ni = label_to_index.at(n);
            const int a = std::min(vi, ni);
            const int b = std::max(vi, ni);
            if (seen_edges.insert({a, b}).second) {
                boost::add_edge(static_cast<unsigned>(a), static_cast<unsigned>(b), bg);
            }
        }
    }

    auto edge_index = get(boost::edge_index, bg);
    std::size_t edge_count = 0;
    typename boost::graph_traits<BoostGraph>::edge_iterator ei, ei_end;
    for (boost::tie(ei, ei_end) = boost::edges(bg); ei != ei_end; ++ei) {
        put(edge_index, *ei, static_cast<int>(edge_count++));
    }

    using EdgeVec = std::vector<typename boost::graph_traits<BoostGraph>::edge_descriptor>;
    std::vector<EdgeVec> embedding(boost::num_vertices(bg));
    if (!boost::boyer_myrvold_planarity_test(
            boost::boyer_myrvold_params::graph = bg,
            boost::boyer_myrvold_params::embedding = &embedding[0])) {
        throw std::invalid_argument("get_planar_faces: graph is not planar");
    }

    FaceCollector vis;
    boost::planar_face_traversal(bg, &embedding[0], vis);

    std::vector<int> index_to_label(static_cast<std::size_t>(boost::num_vertices(bg)));
    for (const auto& kv : label_to_index) {
        index_to_label[static_cast<std::size_t>(kv.second)] = kv.first;
    }

    std::vector<std::vector<int>> out;
    out.reserve(vis.faces.size());
    for (const auto& face : vis.faces) {
        std::vector<int> mapped;
        mapped.reserve(face.size());
        for (int idx : face) {
            mapped.push_back(index_to_label[static_cast<std::size_t>(idx)]);
        }
        // Mirror C# Distinct() while preserving order first occurrence
        std::vector<int> distinct;
        std::set<int> dup;
        for (int x : mapped) {
            if (dup.insert(x).second) {
                distinct.push_back(x);
            }
        }
        out.push_back(std::move(distinct));
    }
    return out;
}

} // namespace edgar::graphs
