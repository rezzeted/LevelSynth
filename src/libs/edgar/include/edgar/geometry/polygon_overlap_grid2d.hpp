#pragma once

// Derived from Edgar-DotNet `PolygonOverlapBase` (MIT) — overlap along line via rectangle partitions + grid sweep.

#include "edgar/geometry/orthogonal_line_grid2d.hpp"
#include "edgar/geometry/polygon_grid2d.hpp"
#include "edgar/geometry/rectangle_grid2d.hpp"
#include "edgar/geometry/vector2_int.hpp"

#include <utility>
#include <vector>

namespace edgar::geometry {

bool rectangles_overlap_open(const RectangleGrid2D& a, const RectangleGrid2D& b);

/// Same semantics as C# `IPolygonOverlap.OverlapAlongLine` for integer grid: events along `line` where overlap toggles.
std::vector<std::pair<Vector2Int, bool>> overlap_along_line_polygon_partition(const PolygonGrid2D& moving_polygon,
                                                                             const PolygonGrid2D& fixed_polygon,
                                                                             const OrthogonalLineGrid2D& line);

} // namespace edgar::geometry
