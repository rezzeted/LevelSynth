#pragma once

#include "edgar/geometry/orthogonal_line_grid2d.hpp"

#include <vector>

namespace edgar::generator::grid2d {

/// Valid relative translations for a moving room (C# `ConfigurationSpaceGrid2D` / `ConfigurationSpace` lines subset).
struct ConfigurationSpaceGrid2D {
    std::vector<geometry::OrthogonalLineGrid2D> lines;
};

} // namespace edgar::generator::grid2d
