#pragma once

#include "edgar/geometry/orthogonal_line_grid2d.hpp"

namespace edgar::generator::grid2d {

struct DoorLineGrid2D {
    geometry::OrthogonalLineGrid2D line{};
    int length{};
};

} // namespace edgar::generator::grid2d
