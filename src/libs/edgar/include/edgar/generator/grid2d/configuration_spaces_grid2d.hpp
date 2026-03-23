#pragma once

#include "edgar/generator/grid2d/configuration_space_grid2d.hpp"
#include "edgar/generator/grid2d/configuration_spaces_generator.hpp"
#include "edgar/generator/grid2d/door_line_grid2d.hpp"
#include "edgar/generator/grid2d/weighted_shape_grid2d.hpp"
#include "edgar/geometry/overlap.hpp"
#include "edgar/geometry/polygon_grid2d.hpp"
#include "edgar/geometry/vector2_int.hpp"

#include <vector>

namespace edgar::generator::grid2d {

/// Facade for configuration-space queries (C# `ConfigurationSpacesGrid2D` subset + door-based generator).
class ConfigurationSpacesGrid2D {
public:
    static bool compatible_non_overlapping(const geometry::PolygonGrid2D& a, geometry::Vector2Int pos_a,
                                           const geometry::PolygonGrid2D& b, geometry::Vector2Int pos_b) {
        return !geometry::polygons_overlap_area(a, pos_a, b, pos_b);
    }

    /// Door-aligned valid translations (uses `ConfigurationSpacesGenerator` + `remove_overlapping_along_lines`).
    static ConfigurationSpaceGrid2D configuration_space_between(const geometry::PolygonGrid2D& moving,
                                                                  const std::vector<DoorLineGrid2D>& moving_doors,
                                                                  const geometry::PolygonGrid2D& fixed,
                                                                  const std::vector<DoorLineGrid2D>& fixed_doors) {
        ConfigurationSpacesGenerator gen;
        return gen.get_configuration_space(moving, moving_doors, fixed, fixed_doors);
    }
};

} // namespace edgar::generator::grid2d
