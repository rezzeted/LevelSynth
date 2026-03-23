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
/// Uses merged axis-aligned rectangle intervals along the scan line when both polygons partition cleanly; otherwise
/// falls back to a per-cell check (same as the historical brute-force path in `detail`).
std::vector<std::pair<Vector2Int, bool>> overlap_along_line_polygon_partition(const PolygonGrid2D& moving_polygon,
                                                                             const PolygonGrid2D& fixed_polygon,
                                                                             const OrthogonalLineGrid2D& line);

namespace detail {

/// Brute-force sweep (one overlap test per grid point on `line`). For tests and regression against merged intervals.
std::vector<std::pair<Vector2Int, bool>> overlap_along_line_polygon_partition_bruteforce(
    const PolygonGrid2D& moving_polygon, const PolygonGrid2D& fixed_polygon, const OrthogonalLineGrid2D& line);

} // namespace detail

} // namespace edgar::geometry
