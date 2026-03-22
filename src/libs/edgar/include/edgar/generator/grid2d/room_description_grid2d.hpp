#pragma once

#include <stdexcept>
#include <vector>

#include "edgar/generator/grid2d/room_template_grid2d.hpp"

namespace edgar::generator::grid2d {

class RoomDescriptionGrid2D {
public:
    RoomDescriptionGrid2D(bool is_corridor, std::vector<RoomTemplateGrid2D> room_templates)
        : is_corridor_(is_corridor), room_templates_(std::move(room_templates)) {
        if (room_templates_.empty()) {
            throw std::invalid_argument("room_templates must not be empty");
        }
    }

    bool is_corridor() const { return is_corridor_; }
    const std::vector<RoomTemplateGrid2D>& room_templates() const { return room_templates_; }

private:
    bool is_corridor_{};
    std::vector<RoomTemplateGrid2D> room_templates_;
};

} // namespace edgar::generator::grid2d
