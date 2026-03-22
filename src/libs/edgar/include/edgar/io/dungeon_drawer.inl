#pragma once

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <vector>

#include "edgar/geometry/polygon_grid2d.hpp"
#include "edgar/geometry/vector2_int.hpp"

namespace edgar::io {
namespace detail {

inline void set_pixel(std::vector<std::uint8_t>& rgba, int w, int h, int x, int y, std::uint8_t r, std::uint8_t g,
                        std::uint8_t b) {
    if (x < 0 || y < 0 || x >= w || y >= h) {
        return;
    }
    const int i = (y * w + x) * 4;
    rgba[static_cast<std::size_t>(i + 0)] = r;
    rgba[static_cast<std::size_t>(i + 1)] = g;
    rgba[static_cast<std::size_t>(i + 2)] = b;
    rgba[static_cast<std::size_t>(i + 3)] = 255;
}

inline bool point_in_polygon(geometry::Vector2Int pt, const std::vector<geometry::Vector2Int>& poly) {
    bool c = false;
    for (std::size_t i = 0, j = poly.size() - 1; i < poly.size(); j = i++) {
        const auto& pi = poly[i];
        const auto& pj = poly[j];
        if (pj.y == pi.y) {
            continue;
        }
        if (((pi.y > pt.y) != (pj.y > pt.y)) &&
            (static_cast<double>(pt.x) <
             static_cast<double>(pj.x - pi.x) * static_cast<double>(pt.y - pi.y) / static_cast<double>(pj.y - pi.y) +
                 static_cast<double>(pi.x))) {
            c = !c;
        }
    }
    return c;
}

inline std::vector<geometry::Vector2Int> world_outline(const geometry::PolygonGrid2D& local, geometry::Vector2Int pos) {
    std::vector<geometry::Vector2Int> w;
    for (const auto& p : local.points()) {
        w.push_back(p + pos);
    }
    return w;
}

} // namespace detail

template <typename TRoom>
void DungeonDrawer<TRoom>::draw_layout_and_save(const generator::grid2d::LayoutGrid2D<TRoom>& layout,
                                               const std::string& path, const DungeonDrawerOptions& options) const {
    if (layout.rooms.empty()) {
        throw std::invalid_argument("layout has no rooms");
    }

    int min_x = std::numeric_limits<int>::max();
    int min_y = std::numeric_limits<int>::max();
    int max_x = std::numeric_limits<int>::min();
    int max_y = std::numeric_limits<int>::min();

    for (const auto& room : layout.rooms) {
        const auto poly = detail::world_outline(room.outline, room.position);
        for (const auto& p : poly) {
            min_x = std::min(min_x, p.x);
            min_y = std::min(min_y, p.y);
            max_x = std::max(max_x, p.x);
            max_y = std::max(max_y, p.y);
        }
    }

    const double pad = options.padding_absolute + options.padding_percentage * std::max(max_x - min_x, max_y - min_y);
    const int bbox_w = std::max(1, max_x - min_x + 1);
    const int bbox_h = std::max(1, max_y - min_y + 1);

    double scale = options.scale;
    if (options.width > 0 && options.height > 0) {
        const double sx = (options.width - 2 * pad) / static_cast<double>(bbox_w);
        const double sy = (options.height - 2 * pad) / static_cast<double>(bbox_h);
        scale = std::min(sx, sy);
    }

    const int img_w = std::max(1, static_cast<int>(std::ceil(bbox_w * scale + 2 * pad)));
    const int img_h = std::max(1, static_cast<int>(std::ceil(bbox_h * scale + 2 * pad)));

    std::vector<std::uint8_t> rgba(static_cast<std::size_t>(img_w * img_h * 4), 0);
    const auto bg_r = static_cast<std::uint8_t>((options.background_rgb >> 16) & 0xFF);
    const auto bg_g = static_cast<std::uint8_t>((options.background_rgb >> 8) & 0xFF);
    const auto bg_b = static_cast<std::uint8_t>(options.background_rgb & 0xFF);
    for (int y = 0; y < img_h; ++y) {
        for (int x = 0; x < img_w; ++x) {
            detail::set_pixel(rgba, img_w, img_h, x, y, bg_r, bg_g, bg_b);
        }
    }

    const double ox = pad - min_x * scale;
    const double oy = pad - min_y * scale;

    const auto fill_r = static_cast<std::uint8_t>((options.room_fill_rgb >> 16) & 0xFF);
    const auto fill_g = static_cast<std::uint8_t>((options.room_fill_rgb >> 8) & 0xFF);
    const auto fill_b = static_cast<std::uint8_t>(options.room_fill_rgb & 0xFF);

    for (const auto& room : layout.rooms) {
        const auto world = detail::world_outline(room.outline, room.position);
        int rmin_x = std::numeric_limits<int>::max();
        int rmin_y = std::numeric_limits<int>::max();
        int rmax_x = std::numeric_limits<int>::min();
        int rmax_y = std::numeric_limits<int>::min();
        for (const auto& p : world) {
            rmin_x = std::min(rmin_x, p.x);
            rmin_y = std::min(rmin_y, p.y);
            rmax_x = std::max(rmax_x, p.x);
            rmax_y = std::max(rmax_y, p.y);
        }
        for (int y = rmin_y; y <= rmax_y; ++y) {
            for (int x = rmin_x; x <= rmax_x; ++x) {
                if (detail::point_in_polygon({x, y}, world)) {
                    const int px = static_cast<int>(std::floor(x * scale + ox));
                    const int py = static_cast<int>(std::floor(y * scale + oy));
                    detail::set_pixel(rgba, img_w, img_h, px, py, fill_r, fill_g, fill_b);
                }
            }
        }
    }

    const auto line_r = static_cast<std::uint8_t>((options.outline_rgb >> 16) & 0xFF);
    const auto line_g = static_cast<std::uint8_t>((options.outline_rgb >> 8) & 0xFF);
    const auto line_b = static_cast<std::uint8_t>(options.outline_rgb & 0xFF);

    for (const auto& room : layout.rooms) {
        const auto world = detail::world_outline(room.outline, room.position);
        for (std::size_t i = 0; i < world.size(); ++i) {
            const auto a = world[i];
            const auto b = world[(i + 1) % world.size()];
            const int dx = (b.x > a.x) - (b.x < a.x);
            const int dy = (b.y > a.y) - (b.y < a.y);
            int x = a.x;
            int y = a.y;
            while (true) {
                const int px = static_cast<int>(std::floor(x * scale + ox));
                const int py = static_cast<int>(std::floor(y * scale + oy));
                detail::set_pixel(rgba, img_w, img_h, px, py, line_r, line_g, line_b);
                if (x == b.x && y == b.y) {
                    break;
                }
                if (a.x == b.x) {
                    y += dy;
                } else {
                    x += dx;
                }
            }
        }
    }

    write_png_rgba(path, img_w, img_h, rgba);
}

} // namespace edgar::io
