#include "edgar/geometry/polygon_overlap_grid2d.hpp"
#include "edgar/geometry/grid_polygon_partitioning.hpp"
#include "edgar/geometry/overlap.hpp"

#include <optional>

namespace edgar::geometry {

bool rectangles_overlap_open(const RectangleGrid2D& a, const RectangleGrid2D& b) {
    return a.a.x < b.b.x && a.b.x > b.a.x && a.a.y < b.b.y && a.b.y > b.a.y;
}

static bool moving_fixed_overlap_at(const PolygonGrid2D& moving, const PolygonGrid2D& fixed, Vector2Int position) {
    const auto mr = partition_orthogonal_polygon_to_rectangles(moving);
    const auto fr = partition_orthogonal_polygon_to_rectangles(fixed);
    // Full `GridPolygonPartitioning` should cover normalized orthogonal outlines; Clipper2 fallback matches
    // `PolygonOverlapBase` robustness if decomposition ever returns empty.
    if (mr.empty() || fr.empty()) {
        return polygons_overlap_area(moving, position, fixed, {0, 0});
    }
    for (const auto& m : mr) {
        const RectangleGrid2D mw{m.a + position, m.b + position};
        for (const auto& f : fr) {
            if (rectangles_overlap_open(mw, f)) {
                return true;
            }
        }
    }
    return false;
}

std::vector<std::pair<Vector2Int, bool>> overlap_along_line_polygon_partition(const PolygonGrid2D& moving_polygon,
                                                                              const PolygonGrid2D& fixed_polygon,
                                                                              const OrthogonalLineGrid2D& line) {
    const auto pts = line.grid_points_inclusive();
    std::vector<std::pair<Vector2Int, bool>> events;
    std::optional<bool> prev;
    for (const Vector2Int& p : pts) {
        const bool ov = moving_fixed_overlap_at(moving_polygon, fixed_polygon, p);
        if (!prev.has_value() || ov != *prev) {
            events.push_back({p, ov});
            prev = ov;
        }
    }
    return events;
}

} // namespace edgar::geometry
