#pragma once

// C# `LayoutController` / `SimulatedAnnealingEvolver` subset for Grid2D:
// door-aware greedy placement, perturb shape vs position (0.4 / 0.6),
// energy via `ConstraintsEvaluatorGrid2D`, SA schedule matching C#.
//
// L2 paritet: inner loop order is Perturb -> IsLayoutValid -> IsDifferentEnough ->
// TryCompleteChain on clone -> yield -> Metropolis (independent).
// Random restarts via ShouldRestart(numberOfFailures).

#include "edgar/generator/common/basic_energy_updater.hpp"
#include "edgar/generator/common/simulated_annealing_configuration.hpp"
#include "edgar/generator/grid2d/configuration_spaces_grid2d.hpp"
#include "edgar/generator/grid2d/constraints_evaluator_grid2d.hpp"
#include "edgar/generator/grid2d/detail/room_index_map.hpp"
#include "edgar/generator/grid2d/door_line_grid2d.hpp"
#include "edgar/generator/grid2d/level_description_grid2d.hpp"
#include "edgar/generator/grid2d/grid2d_layout_state.hpp"
#include "edgar/generator/grid2d/layout_orchestration.hpp"
#include "edgar/generator/grid2d/room_template_grid2d.hpp"
#include "edgar/generator/grid2d/simulated_annealing_evolver_grid2d.hpp"
#include "edgar/graphs/undirected_graph.hpp"
#include "edgar/geometry/transformation_grid2d.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <limits>
#include <optional>
#include <random>
#include <vector>

namespace edgar::generator::grid2d {

class LayoutControllerGrid2D {
public:
    explicit LayoutControllerGrid2D(common::SimulatedAnnealingConfiguration config = {}) : config_(std::move(config)) {}

    common::SimulatedAnnealingConfiguration& config() { return config_; }
    const common::SimulatedAnnealingConfiguration& config() const { return config_; }

    template <typename TRoom>
    static std::optional<geometry::Vector2Int> greedy_position_from_configuration_spaces(
        int room_index, const LevelDescriptionGrid2D<TRoom>& level, const detail::RoomIndexMap<TRoom>& rmap,
        const graphs::UndirectedAdjacencyListGraph<int>& ig, const geometry::PolygonGrid2D& moving_outline,
        const std::vector<DoorLineGrid2D>& moving_doors, const std::vector<bool>& placed,
        const std::vector<geometry::PolygonGrid2D>& outlines, const std::vector<geometry::Vector2Int>& positions,
        const std::vector<std::vector<DoorLineGrid2D>>& doors_at_index, std::mt19937& rng) {
        std::vector<int> neigh;
        for (int nb : ig.neighbours(room_index)) {
            if (placed[static_cast<std::size_t>(nb)]) {
                neigh.push_back(nb);
            }
        }
        if (neigh.empty()) {
            return std::nullopt;
        }
        const auto& rd = level.get_room_description(rmap.index_to_room[static_cast<std::size_t>(room_index)]);
        std::vector<bool> corridor_by_index(placed.size());
        for (std::size_t i = 0; i < placed.size(); ++i) {
            corridor_by_index[i] =
                level.get_room_description(rmap.index_to_room[i]).is_corridor();
        }
        return sample_maximum_intersection_position(
            moving_outline, moving_doors, neigh, room_index, outlines, positions, doors_at_index, placed, rng, 120,
            rd.is_corridor(), &corridor_by_index);
    }

    template <typename TRoom>
    static void polish_corridor_positions(const LevelDescriptionGrid2D<TRoom>& level,
                                          const detail::RoomIndexMap<TRoom>& rmap,
                                          const graphs::UndirectedAdjacencyListGraph<int>& ig,
                                          std::vector<geometry::PolygonGrid2D>& outlines,
                                          std::vector<geometry::Vector2Int>& positions,
                                          std::vector<std::optional<RoomTemplateGrid2D>>& templates,
                                          std::mt19937& rng) {
        const int n = static_cast<int>(outlines.size());
        if (n <= 0) {
            return;
        }
        std::vector<bool> placed(static_cast<std::size_t>(n), true);
        std::vector<std::vector<DoorLineGrid2D>> doors_tab(static_cast<std::size_t>(n));
        for (int i = 0; i < n; ++i) {
            const auto& ot = templates[static_cast<std::size_t>(i)];
            if (ot.has_value()) {
                doors_tab[static_cast<std::size_t>(i)] =
                    ot->doors().get_doors(outlines[static_cast<std::size_t>(i)]);
            }
        }
        for (int i = 0; i < n; ++i) {
            if (!level.get_room_description(rmap.index_to_room[static_cast<std::size_t>(i)]).is_corridor()) {
                continue;
            }
            const auto gp = greedy_position_from_configuration_spaces(
                i, level, rmap, ig, outlines[static_cast<std::size_t>(i)],
                doors_tab[static_cast<std::size_t>(i)], placed, outlines, positions, doors_tab, rng);
            if (gp.has_value()) {
                positions[static_cast<std::size_t>(i)] = *gp;
            }
        }
    }

    template <typename TRoom>
    static bool try_complete_chain(const LevelDescriptionGrid2D<TRoom>& level,
                                   const detail::RoomIndexMap<TRoom>& rmap,
                                   const graphs::UndirectedAdjacencyListGraph<int>& ig,
                                   std::vector<geometry::PolygonGrid2D>& outlines,
                                   std::vector<geometry::Vector2Int>& positions,
                                   std::vector<std::optional<RoomTemplateGrid2D>>& templates,
                                   std::mt19937& rng, int max_passes_without_progress, int* iterations_out) {
        const int n = static_cast<int>(outlines.size());
        if (n <= 0) {
            return true;
        }
        std::vector<bool> is_corridor(static_cast<std::size_t>(n));
        for (int i = 0; i < n; ++i) {
            is_corridor[static_cast<std::size_t>(i)] =
                level.get_room_description(rmap.index_to_room[static_cast<std::size_t>(i)]).is_corridor();
        }
        auto total_penalty = [&]() {
            return common::BasicEnergyUpdater::total_penalty(ConstraintsEvaluatorGrid2D::evaluate(
                outlines, positions, level.minimum_room_distance, &is_corridor));
        };
        if (total_penalty() <= 0.0) {
            return true;
        }
        std::vector<bool> placed(static_cast<std::size_t>(n), true);
        int no_progress = 0;
        int sweep_steps = 0;
        while (no_progress < max_passes_without_progress) {
            bool progress = false;
            for (int r = 0; r < n; ++r) {
                std::vector<std::vector<DoorLineGrid2D>> doors_tab(static_cast<std::size_t>(n));
                for (int j = 0; j < n; ++j) {
                    const auto& ot = templates[static_cast<std::size_t>(j)];
                    if (ot.has_value()) {
                        doors_tab[static_cast<std::size_t>(j)] =
                            ot->doors().get_doors(outlines[static_cast<std::size_t>(j)]);
                    }
                }
                const geometry::Vector2Int old_p = positions[static_cast<std::size_t>(r)];
                const auto gp = greedy_position_from_configuration_spaces(
                    r, level, rmap, ig, outlines[static_cast<std::size_t>(r)],
                    doors_tab[static_cast<std::size_t>(r)], placed, outlines, positions, doors_tab, rng);
                if (gp.has_value() && (gp->x != old_p.x || gp->y != old_p.y)) {
                    positions[static_cast<std::size_t>(r)] = *gp;
                    progress = true;
                }
                ++sweep_steps;
            }
            if (total_penalty() <= 0.0) {
                if (iterations_out) {
                    *iterations_out += sweep_steps;
                }
                return true;
            }
            if (!progress) {
                ++no_progress;
            } else {
                no_progress = 0;
            }
        }
        if (iterations_out) {
            *iterations_out += sweep_steps;
        }
        return total_penalty() <= 0.0;
    }

    template <typename TRoom>
    void evolve(const LevelDescriptionGrid2D<TRoom>& level, const detail::RoomIndexMap<TRoom>& rmap,
                const graphs::UndirectedAdjacencyListGraph<int>& ig, std::vector<geometry::PolygonGrid2D>& outlines,
                std::vector<geometry::Vector2Int>& positions,
                std::vector<std::optional<RoomTemplateGrid2D>>& templates,
                std::vector<geometry::TransformationGrid2D>& transforms, std::mt19937& rng, int* iterations_out,
                const ChainGenerateContext<TRoom>* ctx = nullptr,
                Grid2DLayoutState<TRoom>* state_for_inner_clone = nullptr) {
        const int n = static_cast<int>(outlines.size());
        if (n <= 0) {
            if (iterations_out) {
                *iterations_out = 0;
            }
            return;
        }

        std::vector<bool> is_corridor(static_cast<std::size_t>(n));
        for (int i = 0; i < n; ++i) {
            is_corridor[static_cast<std::size_t>(i)] =
                level.get_room_description(rmap.index_to_room[static_cast<std::size_t>(i)]).is_corridor();
        }

        auto doors_at_index = [&]() {
            std::vector<std::vector<DoorLineGrid2D>> tab(static_cast<std::size_t>(n));
            for (int i = 0; i < n; ++i) {
                const auto& ot = templates[static_cast<std::size_t>(i)];
                if (ot.has_value()) {
                    tab[static_cast<std::size_t>(i)] =
                        ot->doors().get_doors(outlines[static_cast<std::size_t>(i)]);
                }
            }
            return tab;
        };

        auto energy = [&]() {
            return common::BasicEnergyUpdater::total_penalty(ConstraintsEvaluatorGrid2D::evaluate(
                outlines, positions, level.minimum_room_distance, &is_corridor));
        };

        auto overlap_total = [&]() {
            return ConstraintsEvaluatorGrid2D::evaluate(
                outlines, positions, level.minimum_room_distance, &is_corridor).overlap_penalty;
        };

        double e = energy();
        double total_overlap = overlap_total();

        constexpr double p0 = 0.2;
        constexpr double p1 = 0.01;
        double t0 = -1.0 / std::log(p0);
        const double t1 = -1.0 / std::log(p1);
        const int cycles = std::max(1, config_.cycles);
        const double ratio =
            (cycles > 1) ? std::pow(t1 / t0, 1.0 / static_cast<double>(cycles - 1)) : 0.9995;
        double delta_e_avg = 0.0;
        int accepted_solutions = 1;
        double t = t0;

        std::uniform_int_distribution<int> pick_room(0, n - 1);
        std::uniform_int_distribution<int> pick_dx(-config_.max_perturbation_radius,
                                                     config_.max_perturbation_radius);
        std::uniform_int_distribution<int> pick_dy(-config_.max_perturbation_radius,
                                                     config_.max_perturbation_radius);
        std::uniform_real_distribution<double> uni01(0.0, 1.0);
        std::uniform_real_distribution<double> shape_vs_pos(0.0, 1.0);

        int iterations = 0;
        int last_event_iterations = 0;
        int inner_tcc_iters_sum = 0;
        int inner_layouts_emitted = 0;
        int number_of_failures = 0;
        int stage_two_failures = 0;
        bool should_stop = false;

        std::vector<bool> placed(static_cast<std::size_t>(n), true);

        auto bb_center = [](const geometry::PolygonGrid2D& poly) -> geometry::Vector2Int {
            const auto& pts = poly.points();
            if (pts.empty()) return {0, 0};
            int min_x = pts[0].x, max_x = pts[0].x;
            int min_y = pts[0].y, max_y = pts[0].y;
            for (std::size_t k = 1; k < pts.size(); ++k) {
                min_x = std::min(min_x, pts[k].x);
                max_x = std::max(max_x, pts[k].x);
                min_y = std::min(min_y, pts[k].y);
                max_y = std::max(max_y, pts[k].y);
            }
            return {(min_x + max_x) / 2, (min_y + max_y) / 2};
        };

        double avg_size = compute_average_room_size(level, rmap);

        struct RoomSnapshot {
            geometry::Vector2Int center;
            const RoomTemplateGrid2D* tmpl;
        };

        std::vector<std::vector<RoomSnapshot>> yielded_snapshots;

        auto make_snapshot = [&]() -> std::vector<RoomSnapshot> {
            std::vector<RoomSnapshot> snap(static_cast<std::size_t>(n));
            for (int idx = 0; idx < n; ++idx) {
                const auto lc = bb_center(outlines[static_cast<std::size_t>(idx)]);
                const auto& pos = positions[static_cast<std::size_t>(idx)];
                snap[static_cast<std::size_t>(idx)] = {
                    {pos.x + lc.x, pos.y + lc.y},
                    templates[static_cast<std::size_t>(idx)].has_value()
                        ? &*templates[static_cast<std::size_t>(idx)] : nullptr
                };
            }
            return snap;
        };

        auto is_different_enough = [&](const std::vector<RoomSnapshot>& candidate) -> bool {
            if (yielded_snapshots.empty()) return true;
            for (const auto& prev : yielded_snapshots) {
                double diff = 0.0;
                for (int idx = 0; idx < n; ++idx) {
                    const auto& c = candidate[static_cast<std::size_t>(idx)];
                    const auto& p = prev[static_cast<std::size_t>(idx)];
                    double center_dist = static_cast<double>(
                        std::abs(c.center.x - p.center.x) + std::abs(c.center.y - p.center.y));
                    double weight = (c.tmpl == p.tmpl) ? 1.0 : 4.0;
                    diff += std::pow(5.0 * center_dist / avg_size, 2.0) * weight;
                }
                diff /= static_cast<double>(n);
                if (0.4 * diff < 1.0) return false;
            }
            return true;
        };

        auto emit_sa = [&](LayoutYieldEvent ev, const Grid2DLayoutState<TRoom>& st, double pen) {
            if (!ctx || !ctx->on_layout) {
                return;
            }
            const bool sa_stream = ctx->layout_stream == LayoutStreamMode::OnEachSaTryCompleteChain;
            if (ev == LayoutYieldEvent::LayoutGenerated && !sa_stream) {
                return;
            }
            if (ev == LayoutYieldEvent::LayoutGenerated) {
                if (ctx->max_layout_yields > 0 && inner_layouts_emitted >= ctx->max_layout_yields) {
                    return;
                }
                ++inner_layouts_emitted;
            }
            LayoutYieldInfo info;
            info.event_type = ev;
            info.iterations_total = iterations + inner_tcc_iters_sum;
            info.energy = pen;
            if (ctx->stats_out) {
                info.iterations_since_last_event = iterations + inner_tcc_iters_sum - last_event_iterations;
                info.layouts_generated = ctx->stats_out->layouts_generated;
                info.chain_number = ctx->stats_out->chain_number;
                ctx->stats_out->iterations_total = iterations + inner_tcc_iters_sum;
            }
            ctx->on_layout(info, st.to_layout_grid());
            if (ctx->stats_out) {
                if (ev == LayoutYieldEvent::LayoutGenerated) {
                    ctx->stats_out->layouts_generated++;
                    ctx->stats_out->iterations_since_last_event = 0;
                } else if (ev == LayoutYieldEvent::StageTwoFailure) {
                    ctx->stats_out->stage_two_failures++;
                }
            }
        };

        for (int i = 0; i < cycles; ++i) {
            if (should_restart(number_of_failures, rng)) {
                if (state_for_inner_clone) {
                    emit_sa(LayoutYieldEvent::RandomRestart, *state_for_inner_clone, e);
                }
                if (iterations_out) {
                    *iterations_out = iterations + inner_tcc_iters_sum;
                }
                return;
            }

            if (iterations - last_event_iterations > config_.max_iterations_without_success) {
                break;
            }

            if (should_stop) {
                break;
            }

            bool was_accepted = false;

            for (int j = 0; j < config_.trials_per_cycle; ++j) {
                if (stage_two_failures > config_.max_stage_two_failures) {
                    should_stop = true;
                    break;
                }

                ++iterations;
                const int r = pick_room(rng);
                const geometry::Vector2Int old_pos = positions[static_cast<std::size_t>(r)];
                const geometry::PolygonGrid2D old_outline = outlines[static_cast<std::size_t>(r)];
                const std::optional<RoomTemplateGrid2D> old_tmpl = templates[static_cast<std::size_t>(r)];
                const geometry::TransformationGrid2D old_tr = transforms[static_cast<std::size_t>(r)];
                bool did_shape_perturb = false;

                const common::EnergyData incident_old =
                    ConstraintsEvaluatorGrid2D::incident_to_room(static_cast<std::size_t>(r), outlines, positions,
                                                                 level.minimum_room_distance, &is_corridor);
                const double incident_old_tot = common::BasicEnergyUpdater::total_penalty(incident_old);

#ifndef NDEBUG
                if (iterations % 256 == 0) {
                    const double full = energy();
                    assert(std::abs(full - e) < 1e-4);
                }
#endif

                if (shape_vs_pos(rng) < 0.4) {
                    const TRoom rid = rmap.index_to_room[static_cast<std::size_t>(r)];
                    const auto& rd = level.get_room_description(rid);
                    const auto& tmpls = rd.room_templates();
                    if (!tmpls.empty()) {
                        std::uniform_int_distribution<std::size_t> pick_t(0, tmpls.size() - 1);
                        const RoomTemplateGrid2D& tmpl = tmpls[pick_t(rng)];
                        const auto& trs = tmpl.allowed_transformations();
                        geometry::TransformationGrid2D tr = geometry::TransformationGrid2D::Identity;
                        if (!trs.empty()) {
                            std::uniform_int_distribution<std::size_t> pick_tr(0, trs.size() - 1);
                            tr = trs[pick_tr(rng)];
                        }
                        outlines[static_cast<std::size_t>(r)] = tmpl.outline().transform(tr);
                        templates[static_cast<std::size_t>(r)] = tmpl;
                        transforms[static_cast<std::size_t>(r)] = tr;
                        did_shape_perturb = true;
                    }
                    const auto doors_tab = doors_at_index();
                    const std::vector<DoorLineGrid2D>& my_doors = doors_tab[static_cast<std::size_t>(r)];
                    std::vector<int> neigh;
                    for (int nb : ig.neighbours(r)) {
                        neigh.push_back(nb);
                    }
                    if (!neigh.empty() && !my_doors.empty()) {
                        const auto np = sample_maximum_intersection_position(
                            outlines[static_cast<std::size_t>(r)], my_doors, neigh, r, outlines, positions,
                            doors_tab, placed, rng, 160, is_corridor[static_cast<std::size_t>(r)], &is_corridor);
                        if (np.has_value()) {
                            positions[static_cast<std::size_t>(r)] = *np;
                        }
                    }
                } else {
                    const auto doors_tab = doors_at_index();
                    const std::vector<DoorLineGrid2D>& my_doors = doors_tab[static_cast<std::size_t>(r)];
                    std::vector<int> neigh;
                    for (int nb : ig.neighbours(r)) {
                        neigh.push_back(nb);
                    }
                    if (!neigh.empty() && !my_doors.empty()) {
                        const auto np = sample_maximum_intersection_position(
                            outlines[static_cast<std::size_t>(r)], my_doors, neigh, r, outlines, positions,
                            doors_tab, placed, rng, 160, is_corridor[static_cast<std::size_t>(r)], &is_corridor);
                        if (np.has_value()) {
                            positions[static_cast<std::size_t>(r)] = *np;
                        } else {
                            positions[static_cast<std::size_t>(r)] = {
                                old_pos.x + pick_dx(rng), old_pos.y + pick_dy(rng)};
                        }
                    } else {
                        positions[static_cast<std::size_t>(r)] = {
                            old_pos.x + pick_dx(rng), old_pos.y + pick_dy(rng)};
                    }
                }

                const common::EnergyData incident_new =
                    ConstraintsEvaluatorGrid2D::incident_to_room(static_cast<std::size_t>(r), outlines, positions,
                                                                 level.minimum_room_distance, &is_corridor);
                const double new_e =
                    e - incident_old_tot + common::BasicEnergyUpdater::total_penalty(incident_new);
                const double energy_delta = new_e - e;

                // C# order: IsLayoutValid -> IsDifferentEnough -> TryCompleteChain on clone -> yield
                // BEFORE Metropolis (which is independent).
                const double new_overlap = total_overlap - incident_old.overlap_penalty + incident_new.overlap_penalty;
                const bool is_valid = (new_overlap <= 0.0);

                if (is_valid) {
                    auto snap = make_snapshot();
                    if (is_different_enough(snap)) {
                        if (state_for_inner_clone) {
                            Grid2DLayoutState<TRoom> cl = state_for_inner_clone->clone();
                            int tcc_iters = 0;
                            const int tcc_pass = std::min(64, std::max(8, n));
                            const bool tcc_ok =
                                try_complete_chain(cl, rng, tcc_pass, &tcc_iters);
                            inner_tcc_iters_sum += tcc_iters;
                            if (ctx && ctx->stats_out) {
                                ctx->stats_out->iterations_since_last_event += tcc_iters;
                            }
                            const double pen_after =
                                common::BasicEnergyUpdater::total_penalty(ConstraintsEvaluatorGrid2D::evaluate(
                                    cl.outlines, cl.positions, level.minimum_room_distance, &is_corridor));
                            if (tcc_ok) {
                                yielded_snapshots.push_back(std::move(snap));
                                emit_sa(LayoutYieldEvent::LayoutGenerated, cl, pen_after);
                                last_event_iterations = iterations + inner_tcc_iters_sum;
                                stage_two_failures = 0;
                            } else {
                                stage_two_failures++;
                                emit_sa(LayoutYieldEvent::StageTwoFailure, cl, pen_after);
                            }
                        }
                    }
                }

                // Metropolis accept/reject (independent of TCC above)
                const double delta_abs = std::abs(energy_delta);
                bool accept = false;
                if (energy_delta > 0.0) {
                    if (i == 0 && j == 0) {
                        delta_e_avg = delta_abs * 15.0;
                    }
                    const double p = std::exp(-delta_abs / (delta_e_avg * t));
                    if (uni01(rng) < p) {
                        accept = true;
                    }
                } else {
                    accept = true;
                }

                if (accept) {
                    ++accepted_solutions;
                    e = new_e;
                    total_overlap = new_overlap;
                    delta_e_avg = (delta_e_avg * static_cast<double>(accepted_solutions - 1) + delta_abs) /
                                  static_cast<double>(accepted_solutions);
                    was_accepted = true;
                } else {
                    positions[static_cast<std::size_t>(r)] = old_pos;
                    if (did_shape_perturb) {
                        outlines[static_cast<std::size_t>(r)] = old_outline;
                        templates[static_cast<std::size_t>(r)] = old_tmpl;
                        transforms[static_cast<std::size_t>(r)] = old_tr;
                    }
                }

                if (e <= 0.0) {
                    if (iterations_out) {
                        *iterations_out = iterations + inner_tcc_iters_sum;
                    }
                    return;
                }
            }

            if (!was_accepted) {
                number_of_failures++;
            }
            t *= ratio;
        }

        if (state_for_inner_clone && ctx && ctx->on_layout) {
            emit_sa(LayoutYieldEvent::OutOfIterations, *state_for_inner_clone, e);
        }

        if (iterations_out) {
            *iterations_out = iterations + inner_tcc_iters_sum;
        }
    }

    void evolve_random_walk(std::vector<geometry::PolygonGrid2D>& outlines,
                            std::vector<geometry::Vector2Int>& positions, std::mt19937& rng,
                            int* iterations_out) const {
        SimulatedAnnealingEvolverGrid2D ev(config_);
        ev.evolve(outlines, positions, rng, iterations_out);
    }

    template <typename TRoom>
    static void polish_corridor_positions(Grid2DLayoutState<TRoom>& state, std::mt19937& rng) {
        polish_corridor_positions(*state.level, state.rmap, state.ig, state.outlines, state.positions, state.templates,
                                  rng);
    }

    template <typename TRoom>
    static bool try_complete_chain(Grid2DLayoutState<TRoom>& state, std::mt19937& rng, int max_passes_without_progress,
                                   int* iterations_out) {
        return try_complete_chain(*state.level, state.rmap, state.ig, state.outlines, state.positions, state.templates,
                                  rng, max_passes_without_progress, iterations_out);
    }

    template <typename TRoom>
    void evolve(Grid2DLayoutState<TRoom>& state, std::mt19937& rng, int* iterations_out,
                 const ChainGenerateContext<TRoom>* ctx = nullptr) {
        evolve(*state.level, state.rmap, state.ig, state.outlines, state.positions, state.templates, state.transforms,
               rng, iterations_out, ctx, &state);
    }

private:
    common::SimulatedAnnealingConfiguration config_;

    static bool should_restart(int number_of_failures, std::mt19937& rng) {
        std::uniform_int_distribution<int> dist;
        if (number_of_failures > 8 && dist(rng, std::uniform_int_distribution<int>::param_type{0, 1}) == 0)
            return true;
        if (number_of_failures > 6 && dist(rng, std::uniform_int_distribution<int>::param_type{0, 2}) == 0)
            return true;
        if (number_of_failures > 4 && dist(rng, std::uniform_int_distribution<int>::param_type{0, 4}) == 0)
            return true;
        if (number_of_failures > 2 && dist(rng, std::uniform_int_distribution<int>::param_type{0, 6}) == 0)
            return true;
        return false;
    }

    template <typename TRoom>
    static double compute_average_room_size(const LevelDescriptionGrid2D<TRoom>& level,
                                             const detail::RoomIndexMap<TRoom>& rmap) {
        double total = 0.0;
        int count = 0;
        for (std::size_t i = 0; i < rmap.index_to_room.size(); ++i) {
            const auto& rd = level.get_room_description(rmap.index_to_room[i]);
            for (const auto& tmpl : rd.room_templates()) {
                const auto& pts = tmpl.outline().points();
                if (pts.empty()) continue;
                int min_x = pts[0].x, max_x = pts[0].x;
                int min_y = pts[0].y, max_y = pts[0].y;
                for (std::size_t k = 1; k < pts.size(); ++k) {
                    min_x = std::min(min_x, pts[k].x);
                    max_x = std::max(max_x, pts[k].x);
                    min_y = std::min(min_y, pts[k].y);
                    max_y = std::max(max_y, pts[k].y);
                }
                total += static_cast<double>((max_x - min_x) + (max_y - min_y)) / 2.0;
                ++count;
            }
        }
        return (count > 0) ? total / static_cast<double>(count) : 10.0;
    }
};

} // namespace edgar::generator::grid2d
