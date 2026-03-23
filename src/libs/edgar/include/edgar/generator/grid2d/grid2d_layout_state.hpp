#pragma once

#include "edgar/generator/grid2d/detail/room_index_map.hpp"
#include "edgar/generator/grid2d/layout_grid2d.hpp"
#include "edgar/generator/grid2d/level_description_grid2d.hpp"
#include "edgar/generator/grid2d/room_template_grid2d.hpp"
#include "edgar/geometry/transformation_grid2d.hpp"
#include "edgar/graphs/undirected_graph.hpp"

#include <optional>
#include <vector>

namespace edgar::generator::grid2d {

/// Mutable Grid2D placement state (C# `ILayout` / `SmartClone` analogue): one object for outlines,
/// positions, templates, and transforms indexed consistently with `RoomIndexMap`.
template <typename TRoom>
struct Grid2DLayoutState {
    const LevelDescriptionGrid2D<TRoom>* level = nullptr;
    detail::RoomIndexMap<TRoom> rmap;
    graphs::UndirectedAdjacencyListGraph<int> ig;

    std::vector<geometry::PolygonGrid2D> outlines;
    std::vector<geometry::Vector2Int> positions;
    std::vector<std::optional<RoomTemplateGrid2D>> templates;
    std::vector<geometry::TransformationGrid2D> transforms;

    explicit Grid2DLayoutState(const LevelDescriptionGrid2D<TRoom>& lvl)
        : level(&lvl), rmap(lvl), ig(rmap.int_graph(lvl)) {}

    int room_count() const { return static_cast<int>(rmap.index_to_room.size()); }

    void resize_room_slots(int n) {
        outlines.assign(static_cast<std::size_t>(n), geometry::PolygonGrid2D::get_rectangle(1, 1));
        positions.assign(static_cast<std::size_t>(n), geometry::Vector2Int{});
        transforms.assign(static_cast<std::size_t>(n), geometry::TransformationGrid2D::Identity);
        templates.assign(static_cast<std::size_t>(n), std::nullopt);
    }

    Grid2DLayoutState clone() const {
        Grid2DLayoutState copy(*level);
        copy.outlines = outlines;
        copy.positions = positions;
        copy.templates = templates;
        copy.transforms = transforms;
        return copy;
    }

    LayoutGrid2D<TRoom> to_layout_grid() const {
        const int n = room_count();
        LayoutGrid2D<TRoom> layout;
        layout.rooms.reserve(static_cast<std::size_t>(n));
        for (int i = 0; i < n; ++i) {
            const TRoom id = rmap.index_to_room[static_cast<std::size_t>(i)];
            const auto& rd = level->get_room_description(id);
            layout.rooms.push_back(LayoutRoomGrid2D<TRoom>{
                .room = id,
                .outline = outlines[static_cast<std::size_t>(i)],
                .position = positions[static_cast<std::size_t>(i)],
                .is_corridor = rd.is_corridor(),
                .room_template = *templates[static_cast<std::size_t>(i)],
                .room_description = rd,
                .transformation = transforms[static_cast<std::size_t>(i)],
            });
        }
        return layout;
    }
};

} // namespace edgar::generator::grid2d
