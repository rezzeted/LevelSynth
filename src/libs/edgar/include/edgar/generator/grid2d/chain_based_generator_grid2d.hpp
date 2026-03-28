#pragma once

#include "edgar/chain_decompositions/breadth_first_chain_decomposition.hpp"
#include "edgar/chain_decompositions/breadth_first_chain_decomposition_old.hpp"
#include "edgar/chain_decompositions/two_stage_chain_decomposition.hpp"
#include "edgar/generator/common/simulated_annealing_configuration.hpp"
#include "edgar/generator/grid2d/graph_based_generator_configuration.hpp"
#include "edgar/generator/grid2d/configuration_spaces_grid2d.hpp"
#include "edgar/generator/grid2d/detail/room_index_map.hpp"
#include "edgar/generator/grid2d/grid2d_layout_state.hpp"
#include "edgar/generator/grid2d/layout_grid2d.hpp"
#include "edgar/generator/grid2d/layout_orchestration.hpp"
#include "edgar/generator/common/basic_energy_updater.hpp"
#include "edgar/generator/grid2d/constraints_evaluator_grid2d.hpp"
#include "edgar/generator/grid2d/level_description_grid2d.hpp"
#include "edgar/generator/grid2d/layout_controller_grid2d.hpp"
#include "edgar/geometry/transformation_grid2d.hpp"

#include <algorithm>
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
                           chain_decompositions::ChainDecompositionConfiguration chain_cfg = {},
                           const ChainGenerateContext<TRoom>* ctx = nullptr) {
        Grid2DLayoutState<TRoom> state(level);
        const detail::RoomIndexMap<TRoom>& rmap = state.rmap;
        const auto& ig = state.ig;

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

        const bool is_tree_graph = edgar::graphs::is_tree(ig);
        const bool use_greedy_tree = sa_config.handle_trees_greedily && is_tree_graph;

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

        std::vector<bool> is_corridor_flags(static_cast<std::size_t>(n));
        for (int i = 0; i < n; ++i) {
            is_corridor_flags[static_cast<std::size_t>(i)] =
                level.get_room_description(rmap.index_to_room[static_cast<std::size_t>(i)]).is_corridor();
        }

        auto penalty_total = [&](const std::vector<geometry::PolygonGrid2D>& ol,
                                 const std::vector<geometry::Vector2Int>& pos) {
            return common::BasicEnergyUpdater::total_penalty(
                ConstraintsEvaluatorGrid2D::evaluate(ol, pos, level.minimum_room_distance, &is_corridor_flags));
        };

        const int max_layout_restarts =
            std::max(1, std::min(sa_config.max_stage_two_failures, 128));
        int iter_count = 0;
        int yields_emitted = 0;
        const int max_yields = ctx ? ctx->max_layout_yields : 0;

        auto sync_stats_iterations = [&]() {
            if (ctx && ctx->stats_out) {
                ctx->stats_out->iterations_total = iter_count;
            }
        };

        auto emit = [&](LayoutYieldEvent ev, Grid2DLayoutState<TRoom>& st, double pen) {
            if (!ctx || !ctx->on_layout) {
                return;
            }
            const bool outer_stream = ctx->layout_stream == LayoutStreamMode::OnEachLayoutGenerated;
            const bool sa_stream = ctx->layout_stream == LayoutStreamMode::OnEachSaTryCompleteChain;
            if (!outer_stream && !sa_stream) {
                return;
            }
            if (ev == LayoutYieldEvent::LayoutGenerated && !outer_stream) {
                return;
            }
            if (ev == LayoutYieldEvent::LayoutGenerated) {
                if (max_yields > 0 && yields_emitted >= max_yields) {
                    return;
                }
                ++yields_emitted;
            }
            LayoutYieldInfo info;
            info.event_type = ev;
            info.iterations_total = iter_count;
            info.energy = pen;
            if (ctx->stats_out) {
                info.iterations_since_last_event = ctx->stats_out->iterations_since_last_event;
                info.layouts_generated = ctx->stats_out->layouts_generated;
                info.chain_number = ctx->stats_out->chain_number;
            }
            sync_stats_iterations();
            ctx->on_layout(info, st.to_layout_grid());
            if (ctx->stats_out && ev == LayoutYieldEvent::LayoutGenerated) {
                ctx->stats_out->layouts_generated++;
                ctx->stats_out->iterations_since_last_event = 0;
            }
        };

        bool success = false;
        double last_penalty = 0.0;

        for (int restart = 0; restart < max_layout_restarts; ++restart) {
            if (restart > 0) {
                if (ctx && ctx->stats_out) {
                    ctx->stats_out->number_of_failures++;
                }
                emit(LayoutYieldEvent::RandomRestart, state, last_penalty);
            }

            state.resize_room_slots(n);
            auto& outlines = state.outlines;
            auto& positions = state.positions;
            auto& transforms = state.transforms;
            auto& templates = state.templates;
            std::vector<bool> placed(static_cast<std::size_t>(n), false);

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
                    if (ctx && ctx->stats_out) {
                        ctx->stats_out->iterations_since_last_event++;
                    }
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
                        if (!ConfigurationSpacesGrid2D::compatible_non_overlapping(
                                outlines[static_cast<std::size_t>(j)], positions[static_cast<std::size_t>(j)],
                                outlines[static_cast<std::size_t>(ri)], pos)) {
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

            LayoutControllerGrid2D::polish_corridor_positions(state, rng);

            if (!use_greedy_tree) {
                LayoutControllerGrid2D controller(sa_config);
                int sa_iters = 0;
                controller.evolve(state, rng, &sa_iters, ctx);
                iter_count += sa_iters;
                if (ctx && ctx->stats_out) {
                    ctx->stats_out->iterations_since_last_event += sa_iters;
                }
            }

            int tcc_iters = 0;
            const int tcc_pass_limit = std::min(64, std::max(8, n));
            LayoutControllerGrid2D::try_complete_chain(state, rng, tcc_pass_limit, &tcc_iters);
            iter_count += tcc_iters;
            if (ctx && ctx->stats_out) {
                ctx->stats_out->iterations_since_last_event += tcc_iters;
            }

            last_penalty = penalty_total(outlines, positions);
            sync_stats_iterations();

            if (last_penalty <= 0.0) {
                success = true;
                emit(LayoutYieldEvent::LayoutGenerated, state, last_penalty);
                break;
            }
            if (ctx && ctx->stats_out) {
                ctx->stats_out->stage_two_failures++;
            }
            emit(LayoutYieldEvent::StageTwoFailure, state, last_penalty);
        }

        if (!success && ctx && ctx->on_layout &&
            (ctx->layout_stream == LayoutStreamMode::OnEachLayoutGenerated ||
             ctx->layout_stream == LayoutStreamMode::OnEachSaTryCompleteChain)) {
            emit(LayoutYieldEvent::OutOfIterations, state, last_penalty);
        }
        sync_stats_iterations();

        LayoutGrid2D<TRoom> layout = state.to_layout_grid();
        return Result{std::move(layout), iter_count};
    }
};

} // namespace edgar::generator::grid2d
