#pragma once

#include "edgar/chain_decompositions/breadth_first_chain_decomposition.hpp"
#include "edgar/chain_decompositions/breadth_first_chain_decomposition_old.hpp"
#include "edgar/chain_decompositions/two_stage_chain_decomposition.hpp"
#include "edgar/generator/common/simulated_annealing_configuration.hpp"
#include "edgar/generator/grid2d/graph_based_generator_configuration.hpp"
#include "edgar/generator/grid2d/configuration_spaces_grid2d.hpp"
#include "edgar/generator/grid2d/detail/room_index_map.hpp"
#include "edgar/generator/grid2d/layout_grid2d.hpp"
#include "edgar/generator/grid2d/level_description_grid2d.hpp"
#include "edgar/generator/grid2d/layout_controller_grid2d.hpp"
#include "edgar/geometry/transformation_grid2d.hpp"

#include <cstddef>
#include <optional>
#include <random>
#include <stdexcept>
#include <tuple>
#include <utility>
#include <vector>

namespace edgar::generator::grid2d {

/// Chain decomposition + initial placement + SA polish (C# `ChainBasedGenerator` / `GraphBasedGeneratorGrid2D` pipeline subset).
template <typename TRoom>
class ChainBasedGeneratorGrid2D {
public:
    struct Result {
        LayoutGrid2D<TRoom> layout;
        int iterations = 0;
    };

    static Result generate(const LevelDescriptionGrid2D<TRoom>& level, common::SimulatedAnnealingConfiguration sa_config,
                           std::mt19937& rng,
                           ChainDecompositionStrategy chain_strategy = ChainDecompositionStrategy::breadth_first_old,
                           chain_decompositions::ChainDecompositionConfiguration chain_cfg = {}) {
        detail::RoomIndexMap<TRoom> rmap(level);
        const auto ig = rmap.int_graph(level);

        std::vector<chain_decompositions::Chain<int>> chains;
        switch (chain_strategy) {
        case ChainDecompositionStrategy::breadth_first_old: {
            chain_decompositions::BreadthFirstChainDecompositionOld decomposer;
            chains = decomposer.get_chains(ig);
            break;
        }
        case ChainDecompositionStrategy::breadth_first_new: {
            chain_decompositions::BreadthFirstChainDecomposition decomposer(std::move(chain_cfg));
            chains = decomposer.get_chains(ig);
            break;
        }
        case ChainDecompositionStrategy::two_stage: {
            chain_decompositions::BreadthFirstChainDecomposition inner(std::move(chain_cfg));
            chain_decompositions::TwoStageChainDecomposition<TRoom> two_stage(level, rmap, inner);
            chains = two_stage.get_chains(ig);
            break;
        }
        default:
            throw std::invalid_argument("ChainBasedGeneratorGrid2D: unknown chain decomposition strategy");
        }

        const int n = static_cast<int>(rmap.index_to_room.size());
        std::vector<int> order;
        order.reserve(static_cast<std::size_t>(n));
        for (const auto& ch : chains) {
            for (int node : ch.nodes) {
                order.push_back(node);
            }
        }
        if (static_cast<int>(order.size()) != n) {
            throw std::runtime_error("ChainBasedGeneratorGrid2D: chain decomposition does not cover all rooms");
        }

        std::vector<geometry::PolygonGrid2D> outlines(static_cast<std::size_t>(n),
                                                        geometry::PolygonGrid2D::get_rectangle(1, 1));
        std::vector<geometry::Vector2Int> positions(static_cast<std::size_t>(n));
        std::vector<geometry::TransformationGrid2D> transforms(static_cast<std::size_t>(n),
                                                                 geometry::TransformationGrid2D::Identity);
        std::vector<std::optional<RoomTemplateGrid2D>> templates(static_cast<std::size_t>(n));
        std::vector<bool> placed(static_cast<std::size_t>(n), false);

        int iter_count = 0;

        auto pick_template = [&](int room_index) {
            const TRoom id = rmap.index_to_room[static_cast<std::size_t>(room_index)];
            const auto& rd = level.get_room_description(id);
            std::uniform_int_distribution<std::size_t> pick(0, rd.room_templates().size() - 1);
            const RoomTemplateGrid2D& tmpl = rd.room_templates()[pick(rng)];
            const auto& trs = tmpl.allowed_transformations();
            const geometry::TransformationGrid2D tr =
                trs.empty() ? geometry::TransformationGrid2D::Identity : trs.front();
            geometry::PolygonGrid2D poly = tmpl.outline().transform(tr);
            return std::tuple{tmpl, rd, poly, tr};
        };

        {
            const int r0 = order[0];
            auto [tmpl, rd, poly, tr] = pick_template(r0);
            (void)rd;
            templates[static_cast<std::size_t>(r0)] = std::move(tmpl);
            outlines[static_cast<std::size_t>(r0)] = std::move(poly);
            transforms[static_cast<std::size_t>(r0)] = tr;
            positions[static_cast<std::size_t>(r0)] = {0, 0};
            placed[static_cast<std::size_t>(r0)] = true;
        }

        std::uniform_int_distribution<int> jitter(-32, 32);

        for (std::size_t k = 1; k < order.size(); ++k) {
            const int ri = order[k];
            int pj = -1;
            for (int nb : ig.neighbours(ri)) {
                if (placed[static_cast<std::size_t>(nb)]) {
                    pj = nb;
                    break;
                }
            }
            if (pj < 0) {
                throw std::runtime_error("ChainBasedGeneratorGrid2D: no placed neighbour");
            }
            auto [tmpl, rd, poly, tr] = pick_template(ri);
            (void)rd;
            templates[static_cast<std::size_t>(ri)] = std::move(tmpl);
            outlines[static_cast<std::size_t>(ri)] = std::move(poly);
            transforms[static_cast<std::size_t>(ri)] = tr;

            bool ok = false;
            for (int attempt = 0; attempt < 8000; ++attempt) {
                ++iter_count;
                std::vector<std::vector<DoorLineGrid2D>> doors_tab(static_cast<std::size_t>(n));
                for (int j = 0; j < n; ++j) {
                    if (!templates[static_cast<std::size_t>(j)].has_value()) {
                        continue;
                    }
                    if (j == ri || placed[static_cast<std::size_t>(j)]) {
                        doors_tab[static_cast<std::size_t>(j)] =
                            templates[static_cast<std::size_t>(j)]->doors().get_doors(
                                outlines[static_cast<std::size_t>(j)]);
                    }
                }
                const auto greedy_pos = LayoutControllerGrid2D::greedy_position_from_configuration_spaces(
                    ri, level, rmap, ig, outlines[static_cast<std::size_t>(ri)],
                    doors_tab[static_cast<std::size_t>(ri)], placed, outlines, positions, doors_tab, rng);
                if (greedy_pos.has_value()) {
                    positions[static_cast<std::size_t>(ri)] = *greedy_pos;
                    placed[static_cast<std::size_t>(ri)] = true;
                    ok = true;
                    break;
                }

                const int dx = jitter(rng);
                const int dy = jitter(rng);
                const geometry::Vector2Int pos{positions[static_cast<std::size_t>(pj)].x + dx,
                                               positions[static_cast<std::size_t>(pj)].y + dy};
                bool bad = false;
                for (int j = 0; j < n; ++j) {
                    if (!placed[static_cast<std::size_t>(j)]) {
                        continue;
                    }
                    if (!ConfigurationSpacesGrid2D::compatible_non_overlapping(outlines[static_cast<std::size_t>(j)],
                                                                             positions[static_cast<std::size_t>(j)],
                                                                             outlines[static_cast<std::size_t>(ri)],
                                                                             pos)) {
                        bad = true;
                        break;
                    }
                }
                if (!bad) {
                    positions[static_cast<std::size_t>(ri)] = pos;
                    placed[static_cast<std::size_t>(ri)] = true;
                    ok = true;
                    break;
                }
            }
            if (!ok) {
                throw std::runtime_error("ChainBasedGeneratorGrid2D: failed to place room without overlap");
            }
        }

        LayoutControllerGrid2D controller(sa_config);
        int sa_iters = 0;
        controller.evolve(level, rmap, ig, outlines, positions, templates, transforms, rng, &sa_iters);
        iter_count += sa_iters;

        LayoutGrid2D<TRoom> layout;
        layout.rooms.reserve(static_cast<std::size_t>(n));
        for (int i = 0; i < n; ++i) {
            const TRoom id = rmap.index_to_room[static_cast<std::size_t>(i)];
            const auto& rd = level.get_room_description(id);
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
        return Result{std::move(layout), iter_count};
    }
};

} // namespace edgar::generator::grid2d
