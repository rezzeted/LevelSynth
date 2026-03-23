#pragma once

#include "edgar/generator/grid2d/configuration_space_grid2d.hpp"
#include "edgar/generator/grid2d/door_line_grid2d.hpp"
#include "edgar/geometry/polygon_grid2d.hpp"

#include <utility>
#include <vector>

namespace edgar::generator::grid2d {

/// Port of C# `ConfigurationSpacesGenerator` (doors, `RemoveOverlapping`, `RemoveIntersections`, corridors).
class ConfigurationSpacesGenerator {
public:
    ConfigurationSpaceGrid2D get_configuration_space(const geometry::PolygonGrid2D& polygon,
                                                     const std::vector<DoorLineGrid2D>& door_lines,
                                                     const geometry::PolygonGrid2D& fixed_center,
                                                     const std::vector<DoorLineGrid2D>& door_lines_fixed,
                                                     const std::vector<int>* offsets = nullptr);

    /// C# `GetConfigurationSpaceOverCorridor` — CS for `polygon` vs fixed room using corridor as movable door carrier.
    ConfigurationSpaceGrid2D get_configuration_space_over_corridor(
        const geometry::PolygonGrid2D& polygon, const std::vector<DoorLineGrid2D>& door_lines,
        const geometry::PolygonGrid2D& fixed_polygon, const std::vector<DoorLineGrid2D>& fixed_door_lines,
        const geometry::PolygonGrid2D& corridor, const std::vector<DoorLineGrid2D>& corridor_door_lines);

    /// C# `GetConfigurationSpaceOverCorridors` — union of per-corridor CS lines, then `RemoveIntersections`.
    ConfigurationSpaceGrid2D get_configuration_space_over_corridors(
        const geometry::PolygonGrid2D& polygon, const std::vector<DoorLineGrid2D>& door_lines,
        const geometry::PolygonGrid2D& fixed_polygon, const std::vector<DoorLineGrid2D>& fixed_door_lines,
        const std::vector<std::pair<geometry::PolygonGrid2D, std::vector<DoorLineGrid2D>>>& corridors);
};

} // namespace edgar::generator::grid2d
