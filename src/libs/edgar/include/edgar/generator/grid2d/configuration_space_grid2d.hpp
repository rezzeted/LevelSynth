#pragma once

#include "edgar/generator/grid2d/door_line_grid2d.hpp"
#include "edgar/geometry/orthogonal_line_grid2d.hpp"

#include <utility>
#include <vector>

namespace edgar::generator::grid2d {

struct ConfigurationSpaceGrid2D {
    std::vector<geometry::OrthogonalLineGrid2D> lines;
    std::vector<std::pair<geometry::OrthogonalLineGrid2D, DoorLineGrid2D>> reverse_doors;
};

} // namespace edgar::generator::grid2d
