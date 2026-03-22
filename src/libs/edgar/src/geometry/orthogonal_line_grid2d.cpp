#include "edgar/geometry/orthogonal_line_grid2d.hpp"

namespace edgar::geometry {

OrthogonalLineGrid2D::OrthogonalLineGrid2D(Vector2Int f, Vector2Int t) : from(f), to(t) {
    if (f.x != t.x && f.y != t.y) {
        throw std::invalid_argument("OrthogonalLineGrid2D: not axis-aligned");
    }
}

OrthogonalDirection OrthogonalLineGrid2D::get_direction() const {
    if (from.x == to.x && from.y == to.y) {
        return OrthogonalDirection::Undefined;
    }
    if (from.x == to.x) {
        return from.y > to.y ? OrthogonalDirection::Bottom : OrthogonalDirection::Top;
    }
    if (from.y == to.y) {
        return from.x > to.x ? OrthogonalDirection::Left : OrthogonalDirection::Right;
    }
    throw std::invalid_argument("OrthogonalLineGrid2D::get_direction: not orthogonal");
}

OrthogonalLineGrid2D OrthogonalLineGrid2D::rotate(int degrees_clockwise) const {
    return OrthogonalLineGrid2D(from.rotate_around_center(degrees_clockwise), to.rotate_around_center(degrees_clockwise));
}

int OrthogonalLineGrid2D::compute_rotation_to_right() const {
    switch (get_direction()) {
        case OrthogonalDirection::Right:
            return 0;
        case OrthogonalDirection::Bottom:
            return 270;
        case OrthogonalDirection::Left:
            return 180;
        case OrthogonalDirection::Top:
            return 90;
        default:
            throw std::invalid_argument("OrthogonalLineGrid2D::compute_rotation_to_right: degenerate line");
    }
}

OrthogonalLineGrid2D OrthogonalLineGrid2D::shrink(int from_amount, int to_amount) const {
    if (length() - from_amount - to_amount < 0) {
        throw std::invalid_argument("OrthogonalLineGrid2D::shrink: not enough length after shrink");
    }
    const int rot = compute_rotation_to_right();
    const OrthogonalLineGrid2D rotated = rotate(rot);
    const Vector2Int moved_from{rotated.from.x + from_amount, rotated.from.y};
    const Vector2Int moved_to{rotated.to.x - to_amount, rotated.to.y};
    return OrthogonalLineGrid2D(moved_from, moved_to).rotate(-rot);
}

} // namespace edgar::geometry
