#include "edgar/geometry/clipper2_util.hpp"

namespace edgar::geometry {

Clipper2Lib::Path64 polygon_to_path64_world(const PolygonGrid2D& poly, Vector2Int world_origin) {
    Clipper2Lib::Path64 path;
    path.reserve(poly.points().size());
    for (const auto& p : poly.points()) {
        path.push_back(Clipper2Lib::Point64(p.x + world_origin.x, p.y + world_origin.y));
    }
    return path;
}

} // namespace edgar::geometry
