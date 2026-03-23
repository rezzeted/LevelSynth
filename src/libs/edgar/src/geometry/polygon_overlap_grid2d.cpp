#include "edgar/geometry/polygon_overlap_grid2d.hpp"
#include "edgar/geometry/grid_polygon_partitioning.hpp"
#include "edgar/geometry/overlap.hpp"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <vector>

namespace edgar::geometry {

bool rectangles_overlap_open(const RectangleGrid2D& a, const RectangleGrid2D& b) {
    return a.a.x < b.b.x && a.b.x > b.a.x && a.a.y < b.b.y && a.b.y > b.a.y;
}

static bool moving_fixed_overlap_at(const PolygonGrid2D& moving, const PolygonGrid2D& fixed, Vector2Int position) {
    const auto mr = partition_orthogonal_polygon_to_rectangles(moving);
    const auto fr = partition_orthogonal_polygon_to_rectangles(fixed);
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

static std::vector<std::pair<int, int>> merge_union_intervals(std::vector<std::pair<int, int>> v) {
    if (v.empty()) {
        return {};
    }
    std::sort(v.begin(), v.end());
    std::vector<std::pair<int, int>> out;
    int lo = v[0].first;
    int hi = v[0].second;
    for (std::size_t i = 1; i < v.size(); ++i) {
        if (v[i].first <= hi + 1) {
            hi = std::max(hi, v[i].second);
        } else {
            out.push_back({lo, hi});
            lo = v[i].first;
            hi = v[i].second;
        }
    }
    out.push_back({lo, hi});
    return out;
}

static bool interval_union_contains(const std::vector<std::pair<int, int>>& merged, int x) {
    for (const auto& iv : merged) {
        if (x >= iv.first && x <= iv.second) {
            return true;
        }
    }
    return false;
}

/// For a horizontal scan line `y_line`, union of integer `tx` where any pair (m,f) has open overlap after shift (tx,
/// y_line). Matches `rectangles_overlap_open` on merged rectangle partitions.
static std::vector<std::pair<int, int>> overlap_tx_intervals_horizontal(const std::vector<RectangleGrid2D>& mr,
                                                                        const std::vector<RectangleGrid2D>& fr,
                                                                        int y_line, int tx_min, int tx_max) {
    std::vector<std::pair<int, int>> raw;
    raw.reserve(mr.size() * fr.size());
    for (const auto& m : mr) {
        for (const auto& f : fr) {
            if (m.a.y + y_line >= f.b.y || m.b.y + y_line <= f.a.y) {
                continue;
            }
            const int lo = f.a.x - m.b.x + 1;
            const int hi = f.b.x - m.a.x - 1;
            const int cl = std::max(lo, tx_min);
            const int cr = std::min(hi, tx_max);
            if (cl <= cr) {
                raw.push_back({cl, cr});
            }
        }
    }
    return merge_union_intervals(std::move(raw));
}

static std::vector<std::pair<int, int>> overlap_ty_intervals_vertical(const std::vector<RectangleGrid2D>& mr,
                                                                        const std::vector<RectangleGrid2D>& fr,
                                                                        int x_line, int ty_min, int ty_max) {
    std::vector<std::pair<int, int>> raw;
    raw.reserve(mr.size() * fr.size());
    for (const auto& m : mr) {
        for (const auto& f : fr) {
            if (m.a.x + x_line >= f.b.x || m.b.x + x_line <= f.a.x) {
                continue;
            }
            const int lo = f.a.y - m.b.y + 1;
            const int hi = f.b.y - m.a.y - 1;
            const int cl = std::max(lo, ty_min);
            const int cr = std::min(hi, ty_max);
            if (cl <= cr) {
                raw.push_back({cl, cr});
            }
        }
    }
    return merge_union_intervals(std::move(raw));
}

static std::vector<std::pair<Vector2Int, bool>> events_along_line_merged(const std::vector<RectangleGrid2D>& mr,
                                                                         const std::vector<RectangleGrid2D>& fr,
                                                                         const OrthogonalLineGrid2D& line) {
    const auto pts = line.grid_points_inclusive();
    std::vector<std::pair<Vector2Int, bool>> events;
    std::optional<bool> prev;

    if (line.from.x == line.to.x) {
        const int x_line = line.from.x;
        const int ty_min = std::min(line.from.y, line.to.y);
        const int ty_max = std::max(line.from.y, line.to.y);
        const auto merged = overlap_ty_intervals_vertical(mr, fr, x_line, ty_min, ty_max);
        for (const Vector2Int& p : pts) {
            const bool ov = interval_union_contains(merged, p.y);
            if (!prev.has_value() || ov != *prev) {
                events.push_back({p, ov});
                prev = ov;
            }
        }
        return events;
    }

    if (line.from.y == line.to.y) {
        const int y_line = line.from.y;
        const int tx_min = std::min(line.from.x, line.to.x);
        const int tx_max = std::max(line.from.x, line.to.x);
        const auto merged = overlap_tx_intervals_horizontal(mr, fr, y_line, tx_min, tx_max);
        for (const Vector2Int& p : pts) {
            const bool ov = interval_union_contains(merged, p.x);
            if (!prev.has_value() || ov != *prev) {
                events.push_back({p, ov});
                prev = ov;
            }
        }
        return events;
    }

    return events;
}

namespace detail {

std::vector<std::pair<Vector2Int, bool>> overlap_along_line_polygon_partition_bruteforce(
    const PolygonGrid2D& moving_polygon, const PolygonGrid2D& fixed_polygon, const OrthogonalLineGrid2D& line) {
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

} // namespace detail

std::vector<std::pair<Vector2Int, bool>> overlap_along_line_polygon_partition(const PolygonGrid2D& moving_polygon,
                                                                              const PolygonGrid2D& fixed_polygon,
                                                                              const OrthogonalLineGrid2D& line) {
    const auto mr = partition_orthogonal_polygon_to_rectangles(moving_polygon);
    const auto fr = partition_orthogonal_polygon_to_rectangles(fixed_polygon);
    if (mr.empty() || fr.empty()) {
        return detail::overlap_along_line_polygon_partition_bruteforce(moving_polygon, fixed_polygon, line);
    }
    return events_along_line_merged(mr, fr, line);
}

} // namespace edgar::geometry
