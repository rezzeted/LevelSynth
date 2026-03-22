#pragma once

#include <cstdint>

namespace edgar::io {

struct DungeonDrawerOptions {
    int width = 2000;
    int height = 2000;
    double scale = 1.0;
    double padding_absolute = 4.0;
    double padding_percentage = 0.02;
    std::uint32_t background_rgb = 0xFFFFFF;
    std::uint32_t room_fill_rgb = 0xE8E8E8;
    std::uint32_t outline_rgb = 0x323232;
    bool enable_shading = false;
    bool enable_grid_lines = false;
};

} // namespace edgar::io
