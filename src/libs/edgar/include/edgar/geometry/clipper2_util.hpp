#pragma once

#include "edgar/geometry/polygon_grid2d.hpp"
#include "edgar/geometry/vector2_int.hpp"

#include <clipper2/clipper.h>

namespace edgar::geometry {

/// Converts a clockwise orthogonal polygon to a Clipper2 path in world space.
Clipper2Lib::Path64 polygon_to_path64_world(const PolygonGrid2D& poly, Vector2Int world_origin);

} // namespace edgar::geometry
