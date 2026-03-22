#pragma once

#include "edgar/generator/grid2d/level_description_grid2d.hpp"
#include "edgar/graphs/undirected_graph.hpp"

#include <algorithm>
#include <map>
#include <vector>

namespace edgar::generator::grid2d::detail {

template <typename TRoom>
struct RoomIndexMap {
    std::vector<TRoom> index_to_room;
    std::map<TRoom, int> room_to_index;

    explicit RoomIndexMap(const LevelDescriptionGrid2D<TRoom>& level) {
        for (const auto& kv : level.rooms()) {
            index_to_room.push_back(kv.first);
        }
        std::sort(index_to_room.begin(), index_to_room.end());
        for (int i = 0; i < static_cast<int>(index_to_room.size()); ++i) {
            room_to_index[index_to_room[static_cast<std::size_t>(i)]] = i;
        }
    }

    graphs::UndirectedAdjacencyListGraph<int> int_graph(const LevelDescriptionGrid2D<TRoom>& level) const {
        graphs::UndirectedAdjacencyListGraph<int> g;
        const auto ug = level.get_graph();
        for (int i = 0; i < static_cast<int>(index_to_room.size()); ++i) {
            g.add_vertex(i);
        }
        for (const auto& kv : room_to_index) {
            const TRoom& v = kv.first;
            const int vi = kv.second;
            for (const TRoom& n : ug.neighbours(v)) {
                const int ni = room_to_index.at(n);
                if (vi < ni) {
                    g.add_edge(vi, ni);
                }
            }
        }
        return g;
    }
};

} // namespace edgar::generator::grid2d::detail
