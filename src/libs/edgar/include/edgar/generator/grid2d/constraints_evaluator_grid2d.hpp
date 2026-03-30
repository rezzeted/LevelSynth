#pragma once

#include "edgar/generator/grid2d/constraints/basic_constraint_grid2d.hpp"
#include "edgar/generator/grid2d/constraints/corridor_constraint_grid2d.hpp"
#include "edgar/generator/grid2d/constraints/minimum_distance_constraint_grid2d.hpp"
#include "edgar/generator/common/energy_data.hpp"
#include "edgar/geometry/polygon_grid2d.hpp"
#include "edgar/geometry/vector2_int.hpp"

#include <algorithm>
#include <cstddef>
#include <vector>

namespace edgar::generator::grid2d {

/// C# `ConstraintsEvaluator` — overlap, optional corridor / minimum-distance terms.
class ConstraintsEvaluatorGrid2D {
public:
    /// Single undirected pair \((i, j)\), \(i < j\) — same contribution as one iteration of `evaluate`.
    static common::EnergyData evaluate_pair(std::size_t i, std::size_t j,
                                            const std::vector<geometry::PolygonGrid2D>& outlines,
                                            const std::vector<geometry::Vector2Int>& positions,
                                            int minimum_room_distance = 0,
                                            const std::vector<bool>* is_corridor = nullptr,
                                            bool optimize_corridor_constraints = true) {
        common::EnergyData out;
        const common::EnergyData basic =
            constraints::BasicConstraintGrid2D::evaluate_pair(i, j, outlines, positions);
        const common::EnergyData corridor = constraints::CorridorConstraintGrid2D::evaluate_pair(
            i, j, basic, is_corridor, optimize_corridor_constraints);
        const common::EnergyData min_distance = constraints::MinimumDistanceConstraintGrid2D::evaluate_pair(
            i, j, outlines, positions, minimum_room_distance);
        out.overlap_penalty += basic.overlap_penalty;
        out.corridor_penalty += corridor.corridor_penalty;
        out.minimum_distance_penalty += min_distance.minimum_distance_penalty;
        return out;
    }

    /// Sum of `evaluate_pair` over all pairs incident to room `r` (C# `UpdateNodeEnergy` aggregate for one node).
    static common::EnergyData incident_to_room(std::size_t r, const std::vector<geometry::PolygonGrid2D>& outlines,
                                               const std::vector<geometry::Vector2Int>& positions,
                                               int minimum_room_distance = 0,
                                               const std::vector<bool>* is_corridor = nullptr,
                                               bool optimize_corridor_constraints = true) {
        common::EnergyData out;
        for (std::size_t j = 0; j < outlines.size(); ++j) {
            if (j == r) {
                continue;
            }
            const std::size_t a = std::min(r, j);
            const std::size_t b = std::max(r, j);
            const common::EnergyData p =
                evaluate_pair(a, b, outlines, positions, minimum_room_distance, is_corridor,
                              optimize_corridor_constraints);
            out.overlap_penalty += p.overlap_penalty;
            out.corridor_penalty += p.corridor_penalty;
            out.minimum_distance_penalty += p.minimum_distance_penalty;
        }
        return out;
    }

    static common::EnergyData evaluate(const std::vector<geometry::PolygonGrid2D>& outlines,
                                       const std::vector<geometry::Vector2Int>& positions,
                                       int minimum_room_distance = 0,
                                       const std::vector<bool>* is_corridor = nullptr,
                                       bool optimize_corridor_constraints = true) {
        common::EnergyData out;
        for (std::size_t i = 0; i < outlines.size(); ++i) {
            for (std::size_t j = i + 1; j < outlines.size(); ++j) {
                const common::EnergyData p =
                    evaluate_pair(i, j, outlines, positions, minimum_room_distance, is_corridor,
                                  optimize_corridor_constraints);
                out.overlap_penalty += p.overlap_penalty;
                out.corridor_penalty += p.corridor_penalty;
                out.minimum_distance_penalty += p.minimum_distance_penalty;
            }
        }
        return out;
    }
};

} // namespace edgar::generator::grid2d
