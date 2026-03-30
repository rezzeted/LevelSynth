#include "edgar/generator/grid2d/simple_door_mode_grid2d.hpp"

#include "edgar/geometry/polygon_grid2d.hpp"

namespace edgar::generator::grid2d {

std::vector<DoorLineGrid2D> SimpleDoorModeGrid2D::get_doors(const geometry::PolygonGrid2D& room_shape) const {
    std::vector<DoorLineGrid2D> doors;
    for (const auto& line : room_shape.get_lines()) {
        if (line.length() < 2 * corner_distance_ + door_length_) {
            continue;
        }
        const geometry::OrthogonalLineGrid2D socket_line =
            line.shrink(corner_distance_, corner_distance_ + door_length_);
        doors.push_back(DoorLineGrid2D{
            .line = socket_line,
            .length = door_length_,
            .direction = line.get_direction()});
    }
    return doors;
}

} // namespace edgar::generator::grid2d
