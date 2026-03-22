#pragma once

#include "edgar/generator/common/energy_data.hpp"
#include "edgar/generator/common/simulated_annealing_configuration.hpp"
#include "edgar/generator/grid2d/constraints_evaluator_grid2d.hpp"
#include "edgar/geometry/polygon_grid2d.hpp"
#include "edgar/geometry/vector2_int.hpp"

#include <cmath>
#include <cstddef>
#include <random>
#include <vector>

namespace edgar::generator::grid2d {

/// Perturbs integer positions to reduce overlap energy (C# `SimulatedAnnealingEvolver` subset).
class SimulatedAnnealingEvolverGrid2D {
public:
    explicit SimulatedAnnealingEvolverGrid2D(common::SimulatedAnnealingConfiguration config = {})
        : config_(config) {}

    void evolve(std::vector<geometry::PolygonGrid2D>& outlines, std::vector<geometry::Vector2Int>& positions,
                std::mt19937& rng, int* iterations_out) const {
        int iters = 0;
        auto energy = [&]() {
            return ConstraintsEvaluatorGrid2D::evaluate(outlines, positions).overlap_penalty;
        };
        double e = energy();
        double T = config_.initial_temperature;
        std::uniform_int_distribution<int> pick_room(0, static_cast<int>(positions.size()) - 1);
        std::uniform_int_distribution<int> pick_dx(-config_.max_perturbation_radius, config_.max_perturbation_radius);
        std::uniform_int_distribution<int> pick_dy(-config_.max_perturbation_radius, config_.max_perturbation_radius);
        std::uniform_real_distribution<double> uni01(0.0, 1.0);

        for (int k = 0; k < config_.iterations; ++k) {
            ++iters;
            if (e <= 0) {
                break;
            }
            const int r = pick_room(rng);
            const geometry::Vector2Int old = positions[static_cast<std::size_t>(r)];
            const int dx = pick_dx(rng);
            const int dy = pick_dy(rng);
            if (dx == 0 && dy == 0) {
                T *= config_.cooling_rate;
                continue;
            }
            positions[static_cast<std::size_t>(r)] = {old.x + dx, old.y + dy};
            const double e2 = energy();
            const double de = e2 - e;
            if (de < 0 || uni01(rng) < std::exp(-de / T)) {
                e = e2;
            } else {
                positions[static_cast<std::size_t>(r)] = old;
            }
            T *= config_.cooling_rate;
        }
        if (iterations_out) {
            *iterations_out = iters;
        }
    }

private:
    common::SimulatedAnnealingConfiguration config_;
};

} // namespace edgar::generator::grid2d
