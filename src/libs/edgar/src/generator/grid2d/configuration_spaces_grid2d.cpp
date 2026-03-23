#include "edgar/generator/grid2d/configuration_spaces_grid2d.hpp"

#include "edgar/generator/grid2d/configuration_spaces_generator.hpp"
#include "edgar/geometry/overlap.hpp"

#include <algorithm>

namespace edgar::generator::grid2d {

bool offset_on_configuration_space(geometry::Vector2Int offset, const ConfigurationSpaceGrid2D& space) {
    for (const auto& line : space.lines) {
        if (line.contains_point(offset)) {
            return true;
        }
    }
    return false;
}

std::vector<geometry::Vector2Int> enumerate_configuration_space_offsets(const ConfigurationSpaceGrid2D& space) {
    std::vector<geometry::Vector2Int> out;
    constexpr std::size_t kMaxTotal = 4000;
    for (const auto& line : space.lines) {
        const auto pts = line.grid_points_inclusive();
        std::size_t stride = 1;
        if (pts.size() > 600) {
            stride = pts.size() / 600 + 1;
        }
        for (std::size_t i = 0; i < pts.size() && out.size() < kMaxTotal; i += stride) {
            out.push_back(pts[i]);
        }
    }
    std::sort(out.begin(), out.end(), [](const geometry::Vector2Int& a, const geometry::Vector2Int& b) {
        return a.x < b.x || (a.x == b.x && a.y < b.y);
    });
    out.erase(std::unique(out.begin(), out.end(),
                          [](const geometry::Vector2Int& a, const geometry::Vector2Int& b) {
                              return a.x == b.x && a.y == b.y;
                          }),
              out.end());
    return out;
}

static bool placement_non_overlapping(int moving_index, geometry::Vector2Int candidate_pos,
                                      const std::vector<geometry::PolygonGrid2D>& outlines,
                                      const std::vector<geometry::Vector2Int>& positions,
                                      const std::vector<bool>& placed) {
    for (std::size_t j = 0; j < outlines.size(); ++j) {
        if (static_cast<int>(j) == moving_index || !placed[j]) {
            continue;
        }
        if (geometry::polygons_overlap_area(outlines[static_cast<std::size_t>(moving_index)], candidate_pos,
                                            outlines[j], positions[j])) {
            return false;
        }
    }
    return true;
}

std::optional<geometry::Vector2Int> sample_maximum_intersection_position(
    const geometry::PolygonGrid2D& moving, const std::vector<DoorLineGrid2D>& moving_doors,
    const std::vector<int>& neighbor_indices, int moving_index, const std::vector<geometry::PolygonGrid2D>& outlines,
    const std::vector<geometry::Vector2Int>& positions, const std::vector<std::vector<DoorLineGrid2D>>& neighbor_doors,
    const std::vector<bool>& placed, std::mt19937& rng, std::size_t max_point_checks) {
    if (neighbor_indices.empty()) {
        return std::nullopt;
    }

    ConfigurationSpacesGenerator gen;
    std::vector<ConfigurationSpaceGrid2D> css;
    css.reserve(neighbor_indices.size());
    for (int k : neighbor_indices) {
        css.push_back(gen.get_configuration_space(moving, moving_doors, outlines[static_cast<std::size_t>(k)],
                                                    neighbor_doors[static_cast<std::size_t>(k)]));
    }

    const int n0 = neighbor_indices[0];
    std::vector<geometry::Vector2Int> candidates = enumerate_configuration_space_offsets(css[0]);
    std::shuffle(candidates.begin(), candidates.end(), rng);

    std::size_t inspected = 0;
    for (const geometry::Vector2Int& v : candidates) {
        if (inspected >= max_point_checks) {
            break;
        }
        ++inspected;
        const geometry::Vector2Int pi{positions[static_cast<std::size_t>(n0)].x + v.x,
                                      positions[static_cast<std::size_t>(n0)].y + v.y};
        bool ok = true;
        for (std::size_t t = 1; t < neighbor_indices.size(); ++t) {
            const int nk = neighbor_indices[t];
            const geometry::Vector2Int delta{pi.x - positions[static_cast<std::size_t>(nk)].x,
                                             pi.y - positions[static_cast<std::size_t>(nk)].y};
            if (!offset_on_configuration_space(delta, css[t])) {
                ok = false;
                break;
            }
        }
        if (!ok) {
            continue;
        }
        if (!placement_non_overlapping(moving_index, pi, outlines, positions, placed)) {
            continue;
        }
        return pi;
    }
    return std::nullopt;
}

} // namespace edgar::generator::grid2d
