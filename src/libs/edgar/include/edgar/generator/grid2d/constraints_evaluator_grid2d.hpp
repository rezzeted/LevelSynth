#pragma once

#include "edgar/generator/common/energy_data.hpp"
#include "edgar/geometry/overlap.hpp"
#include "edgar/geometry/polygon_grid2d.hpp"
#include "edgar/geometry/rectangle_grid2d.hpp"
#include "edgar/geometry/vector2_int.hpp"

#include <algorithm>
#include <cstddef>
#include <vector>

namespace edgar::generator::grid2d {

/// C# `ConstraintsEvaluator` — overlap, optional corridor / minimum-distance terms.
class ConstraintsEvaluatorGrid2D {
public:
    static common::EnergyData evaluate(const std::vector<geometry::PolygonGrid2D>& outlines,
                                       const std::vector<geometry::Vector2Int>& positions,
                                       int minimum_room_distance = 0,
                                       const std::vector<bool>* is_corridor = nullptr) {
        common::EnergyData out;
        for (std::size_t i = 0; i < outlines.size(); ++i) {
            for (std::size_t j = i + 1; j < outlines.size(); ++j) {
                const bool overlap =
                    geometry::polygons_overlap_area(outlines[i], positions[i], outlines[j], positions[j]);
                if (overlap) {
                    out.overlap_penalty += 1.0;
                    if (is_corridor != nullptr && (*is_corridor)[i] != (*is_corridor)[j]) {
                        out.corridor_penalty += 1.0;
                    }
                }
                if (minimum_room_distance > 0) {
                    const geometry::RectangleGrid2D ra = outlines[i].bounding_rectangle() + positions[i];
                    const geometry::RectangleGrid2D rb = outlines[j].bounding_rectangle() + positions[j];
                    const int d = axis_aligned_rect_min_distance(ra, rb);
                    if (d < minimum_room_distance) {
                        out.minimum_distance_penalty +=
                            static_cast<double>(minimum_room_distance - d) / static_cast<double>(minimum_room_distance);
                    }
                }
            }
        }
        return out;
    }

private:
    static int axis_aligned_rect_min_distance(const geometry::RectangleGrid2D& ra,
                                              const geometry::RectangleGrid2D& rb) {
        const int dx = std::max(0, std::max(ra.a.x - rb.b.x, rb.a.x - ra.b.x));
        const int dy = std::max(0, std::max(ra.a.y - rb.b.y, rb.a.y - ra.b.y));
        return dx + dy;
    }
};

} // namespace edgar::generator::grid2d
