#pragma once

#include "edgar/geometry/polygon_grid2d.hpp"
#include "edgar/geometry/vector2_int.hpp"

namespace edgar::geometry {

/// True if closed polygon interiors overlap with positive area (Clipper2 boolean intersection).
bool polygons_overlap_area(const PolygonGrid2D& a, Vector2Int pos_a, const PolygonGrid2D& b, Vector2Int pos_b);

} // namespace edgar::geometry
