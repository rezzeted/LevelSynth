#pragma once

#include <stdexcept>

#include "edgar/geometry/vector2_int.hpp"

namespace edgar::geometry {

struct OrthogonalLineGrid2D {
    Vector2Int from{};
    Vector2Int to{};

    OrthogonalLineGrid2D() = default;
    OrthogonalLineGrid2D(Vector2Int f, Vector2Int t) : from(f), to(t) {
        if (f.x != t.x && f.y != t.y) {
            throw std::invalid_argument("OrthogonalLineGrid2D: not axis-aligned");
        }
    }

    int length() const { return Vector2Int::manhattan_distance(from, to); }
};

} // namespace edgar::geometry
