#pragma once

#include "edgar/geometry/overlap.hpp"
#include "edgar/geometry/polygon_grid2d.hpp"
#include "edgar/geometry/vector2_int.hpp"

namespace edgar::generator::grid2d {

/// Grid2D configuration-space helper: validity of relative placements (C# `ConfigurationSpacesGrid2D` subset).
class ConfigurationSpacesGrid2D {
public:
    static bool compatible_non_overlapping(const geometry::PolygonGrid2D& a, geometry::Vector2Int pos_a,
                                           const geometry::PolygonGrid2D& b, geometry::Vector2Int pos_b) {
        return !geometry::polygons_overlap_area(a, pos_a, b, pos_b);
    }
};

} // namespace edgar::generator::grid2d
