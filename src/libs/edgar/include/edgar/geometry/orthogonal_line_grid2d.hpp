#pragma once

#include <stdexcept>

#include "edgar/geometry/vector2_int.hpp"

namespace edgar::geometry {

/// Matches C# OrthogonalLineGrid2D.Direction (excluding degenerate handling in ctor).
enum class OrthogonalDirection {
    Undefined = 0,
    Top = 1,
    Right = 2,
    Bottom = 3,
    Left = 4,
};

struct OrthogonalLineGrid2D {
    Vector2Int from{};
    Vector2Int to{};

    OrthogonalLineGrid2D() = default;
    OrthogonalLineGrid2D(Vector2Int f, Vector2Int t);

    int length() const { return Vector2Int::manhattan_distance(from, to); }

    OrthogonalDirection get_direction() const;

    /// Clockwise rotation around the origin (same convention as C# / Vector2Int::rotate_around_center).
    OrthogonalLineGrid2D rotate(int degrees_clockwise) const;

    /// Removes `from_amount` steps from the From end and `to_amount` from the To end after rotating
    /// the segment to horizontal "Right" (see C# OrthogonalLineGrid2D.Shrink).
    OrthogonalLineGrid2D shrink(int from_amount, int to_amount) const;
    OrthogonalLineGrid2D shrink(int symmetric_amount) const { return shrink(symmetric_amount, symmetric_amount); }

private:
    int compute_rotation_to_right() const;
};

} // namespace edgar::geometry
