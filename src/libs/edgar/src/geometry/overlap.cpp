#include "edgar/geometry/overlap.hpp"

#include "edgar/geometry/clipper2_util.hpp"

#include <clipper2/clipper.h>

#include <cmath>

namespace edgar::geometry {

bool polygons_overlap_area(const PolygonGrid2D& a, Vector2Int pos_a, const PolygonGrid2D& b, Vector2Int pos_b) {
    const Clipper2Lib::Path64 pa = polygon_to_path64_world(a, pos_a);
    const Clipper2Lib::Path64 pb = polygon_to_path64_world(b, pos_b);
    Clipper2Lib::Paths64 subj;
    subj.push_back(pa);
    Clipper2Lib::Paths64 clips;
    clips.push_back(pb);
    const Clipper2Lib::Paths64 inter =
        Clipper2Lib::Intersect(subj, clips, Clipper2Lib::FillRule::NonZero);
    const double area = Clipper2Lib::Area(inter);
    return std::fabs(area) > 1e-9;
}

} // namespace edgar::geometry
