#pragma once

#include <algorithm>
#include <chrono>
#include <climits>
#include <cstddef>
#include <limits>
#include <random>
#include <stdexcept>
#include <utility>
#include <vector>

#include "edgar/generator/grid2d/chain_based_generator_grid2d.hpp"
#include "edgar/geometry/overlap.hpp"

namespace edgar::generator::grid2d {

namespace detail {

inline int min_coord_x(const geometry::PolygonGrid2D& poly) {
    int v = std::numeric_limits<int>::max();
    for (const auto& p : poly.points()) {
        v = std::min(v, p.x);
    }
    return v;
}

inline int min_coord_y(const geometry::PolygonGrid2D& poly) {
    int v = std::numeric_limits<int>::max();
    for (const auto& p : poly.points()) {
        v = std::min(v, p.y);
    }
    return v;
}

inline int max_world_x(const geometry::PolygonGrid2D& poly, geometry::Vector2Int pos) {
    int mx = INT_MIN;
    for (const auto& p : poly.points()) {
        mx = std::max(mx, p.x + pos.x);
    }
    return mx;
}

} // namespace detail

template <typename TRoom>
GraphBasedGeneratorGrid2D<TRoom>::GraphBasedGeneratorGrid2D(const LevelDescriptionGrid2D<TRoom>& level_description,
                                                          GraphBasedGeneratorConfiguration configuration)
    : level_(level_description), configuration_(std::move(configuration)) {}

template <typename TRoom>
void GraphBasedGeneratorGrid2D<TRoom>::inject_random_generator(std::mt19937 rng) {
    rng_ = std::move(rng);
}

template <typename TRoom>
LayoutGrid2D<TRoom> GraphBasedGeneratorGrid2D<TRoom>::generate_layout() {
    const auto t0 = std::chrono::steady_clock::now();
    iterations_count_ = 0;

    std::mt19937 rng = rng_.has_value() ? *rng_ : std::mt19937{std::random_device{}()};

    if (configuration_.backend == GraphBasedGeneratorBackend::chain_simulated_annealing) {
        orchestration_stats_ = {};
        ChainGenerateContext<TRoom> gen_ctx;
        gen_ctx.layout_stream = configuration_.layout_stream_mode;
        gen_ctx.max_layout_yields = configuration_.max_layout_yields;
        gen_ctx.on_layout = layout_yield_callback_;
        gen_ctx.stats_out = &orchestration_stats_;
        const auto res = ChainBasedGeneratorGrid2D<TRoom>::generate(
            level_, configuration_.simulated_annealing, rng, configuration_.chain_decomposition,
            configuration_.chain_decomposition_configuration, &gen_ctx);
        iterations_count_ = res.iterations;
        const auto t1 = std::chrono::steady_clock::now();
        time_total_ms_ = std::chrono::duration<double, std::milli>(t1 - t0).count();
        return res.layout;
    }

    std::vector<TRoom> order;
    order.reserve(level_.rooms().size());
    for (const auto& kv : level_.rooms()) {
        order.push_back(kv.first);
    }
    std::sort(order.begin(), order.end());

    struct Placed {
        geometry::PolygonGrid2D outline;
        geometry::Vector2Int position{};
    };
    std::vector<Placed> placed;
    placed.reserve(order.size());

    int cursor_x = 0;

    LayoutGrid2D<TRoom> result;

    for (const auto& room : order) {
        const auto& room_desc = level_.get_room_description(room);
        const auto& templates = room_desc.room_templates();
        std::uniform_int_distribution<std::size_t> pick(0, templates.size() - 1);
        const RoomTemplateGrid2D& tmpl = templates[pick(rng)];

        const auto& trs = tmpl.allowed_transformations();
        const geometry::TransformationGrid2D tr =
            trs.empty() ? geometry::TransformationGrid2D::Identity : trs.front();
        geometry::PolygonGrid2D outline = tmpl.outline().transform(tr);

        bool placed_ok = false;
        for (int attempt = 0; attempt < 10000; ++attempt) {
            ++iterations_count_;
            const int min_px = detail::min_coord_x(outline);
            const int min_py = detail::min_coord_y(outline);
            const geometry::Vector2Int pos{cursor_x - min_px, -min_py};

            bool overlap = false;
            for (const auto& p : placed) {
                if (geometry::polygons_overlap_area(outline, pos, p.outline, p.position)) {
                    overlap = true;
                    break;
                }
            }
            if (!overlap) {
                placed.push_back(Placed{outline, pos});
                result.rooms.push_back(LayoutRoomGrid2D<TRoom>{
                    .room = room,
                    .outline = outline,
                    .position = pos,
                    .is_corridor = room_desc.is_corridor(),
                    .room_template = tmpl,
                    .room_description = room_desc,
                    .transformation = tr,
                });

                const int mx = detail::max_world_x(outline, pos);
                cursor_x = mx + level_.minimum_room_distance + 1 + configuration_.strip_gap_cells;
                placed_ok = true;
                break;
            }
            ++cursor_x;
        }

        if (!placed_ok) {
            throw std::runtime_error("GraphBasedGeneratorGrid2D: failed to place room without overlap");
        }
    }

    const auto t1 = std::chrono::steady_clock::now();
    time_total_ms_ = std::chrono::duration<double, std::milli>(t1 - t0).count();
    return result;
}

} // namespace edgar::generator::grid2d
