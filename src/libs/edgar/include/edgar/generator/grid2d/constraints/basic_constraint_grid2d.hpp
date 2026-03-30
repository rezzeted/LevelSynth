#pragma once

#include "edgar/generator/common/energy_data.hpp"
#include "edgar/geometry/overlap.hpp"
#include "edgar/geometry/polygon_grid2d.hpp"
#include "edgar/geometry/vector2_int.hpp"

#include <cstddef>
#include <vector>

namespace edgar::generator::grid2d::constraints {

struct BasicConstraintGrid2D {
    static common::EnergyData evaluate_pair(std::size_t i, std::size_t j,
                                            const std::vector<geometry::PolygonGrid2D>& outlines,
                                            const std::vector<geometry::Vector2Int>& positions) {
        common::EnergyData out;
        if (geometry::polygons_overlap_area(outlines[i], positions[i], outlines[j], positions[j])) {
            out.overlap_penalty = 1.0;
        }
        return out;
    }
};

} // namespace edgar::generator::grid2d::constraints
