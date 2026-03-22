#pragma once

#include "edgar/generator/common/energy_data.hpp"
#include "edgar/geometry/overlap.hpp"
#include "edgar/geometry/polygon_grid2d.hpp"
#include "edgar/geometry/vector2_int.hpp"

#include <cstddef>
#include <vector>

namespace edgar::generator::grid2d {

/// Sums pairwise interior overlaps as energy (C# `ConstraintsEvaluator` / `BasicConstraint` overlap term).
class ConstraintsEvaluatorGrid2D {
public:
    static common::EnergyData evaluate(const std::vector<geometry::PolygonGrid2D>& outlines,
                                       const std::vector<geometry::Vector2Int>& positions) {
        common::EnergyData out;
        double penalty = 0;
        for (std::size_t i = 0; i < outlines.size(); ++i) {
            for (std::size_t j = i + 1; j < outlines.size(); ++j) {
                if (geometry::polygons_overlap_area(outlines[i], positions[i], outlines[j], positions[j])) {
                    penalty += 1.0;
                }
            }
        }
        out.overlap_penalty = penalty;
        return out;
    }
};

} // namespace edgar::generator::grid2d
