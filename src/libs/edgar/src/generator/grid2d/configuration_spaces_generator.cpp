#include "edgar/generator/grid2d/configuration_spaces_generator.hpp"
#include "edgar/generator/grid2d/door_utils.hpp"
#include "edgar/geometry/clipper2_util.hpp"
#include "edgar/geometry/orthogonal_line_intersection.hpp"
#include "edgar/geometry/orthogonal_line_grid2d.hpp"
#include "edgar/geometry/vector2_int.hpp"

#include <array>
#include <stdexcept>

namespace edgar::generator::grid2d {

ConfigurationSpaceGrid2D ConfigurationSpacesGenerator::get_configuration_space(
    const geometry::PolygonGrid2D& polygon, const std::vector<DoorLineGrid2D>& door_lines,
    const geometry::PolygonGrid2D& fixed_center, const std::vector<DoorLineGrid2D>& door_lines_fixed,
    const std::vector<int>* offsets) {
    if (offsets != nullptr && offsets->empty()) {
        throw std::invalid_argument("ConfigurationSpacesGenerator: offsets must be non-empty when set");
    }

    std::vector<DoorLineGrid2D> door_lines_m = merge_door_lines(door_lines);
    std::vector<DoorLineGrid2D> door_lines_f = merge_door_lines(door_lines_fixed);

    std::vector<geometry::OrthogonalLineGrid2D> configuration_space_lines;

    std::array<std::vector<DoorLineGrid2D>, 5> lines_by_dir{};
    for (const auto& d : door_lines_f) {
        lines_by_dir[static_cast<int>(d.line.get_direction())].push_back(d);
    }

    for (const auto& door_line : door_lines_m) {
        const geometry::OrthogonalLineGrid2D& line = door_line.line;
        const geometry::OrthogonalDirection opposite_direction = geometry::opposite_direction(line.get_direction());
        const int rotation = line.compute_rotation_clockwise_degrees();
        const geometry::OrthogonalLineGrid2D rotated_line = line.rotate(rotation);

        for (const auto& c_door_line_fixed : lines_by_dir[static_cast<int>(opposite_direction)]) {
            if (c_door_line_fixed.length != door_line.length) {
                continue;
            }
            const geometry::OrthogonalLineGrid2D cline = c_door_line_fixed.line.rotate(rotation);
            const int y = cline.from.y - rotated_line.from.y;
            const geometry::Vector2Int from(cline.from.x - rotated_line.to.x + (rotated_line.length() - door_line.length), y);
            const geometry::Vector2Int to(cline.to.x - rotated_line.from.x - (rotated_line.length() + door_line.length), y);

            if (from.x < to.x) {
                continue;
            }

            if (offsets == nullptr) {
                geometry::OrthogonalLineGrid2D result_line(from, to);
                configuration_space_lines.push_back(result_line.rotate(-rotation));
            } else {
                for (int offset : *offsets) {
                    const geometry::Vector2Int offset_vector{0, offset};
                    geometry::OrthogonalLineGrid2D result_line(from - offset_vector, to - offset_vector);
                    configuration_space_lines.push_back(result_line.rotate(-rotation));
                }
            }
        }
    }

    configuration_space_lines =
        geometry::remove_overlapping_along_lines(polygon, fixed_center, configuration_space_lines);

    configuration_space_lines = geometry::OrthogonalLineIntersection::remove_intersections(configuration_space_lines);

    return ConfigurationSpaceGrid2D{std::move(configuration_space_lines)};
}

ConfigurationSpaceGrid2D ConfigurationSpacesGenerator::get_configuration_space_over_corridor(
    const geometry::PolygonGrid2D& polygon, const std::vector<DoorLineGrid2D>& door_lines,
    const geometry::PolygonGrid2D& fixed_polygon, const std::vector<DoorLineGrid2D>& fixed_door_lines,
    const geometry::PolygonGrid2D& corridor, const std::vector<DoorLineGrid2D>& corridor_door_lines) {
    const ConfigurationSpaceGrid2D fixed_and_corridor_cs =
        get_configuration_space(corridor, corridor_door_lines, fixed_polygon, fixed_door_lines);

    std::vector<DoorLineGrid2D> merged_corridor_doors = merge_door_lines(corridor_door_lines);
    std::vector<DoorLineGrid2D> new_corridor_door_lines;

    for (const auto& corridor_position_line : fixed_and_corridor_cs.lines) {
        for (const auto& corridor_door_line : merged_corridor_doors) {
            const int rotation = corridor_door_line.line.compute_rotation_clockwise_degrees();
            const geometry::OrthogonalLineGrid2D rotated_line = corridor_door_line.line.rotate(rotation);
            const geometry::OrthogonalLineGrid2D rotated_corridor_line =
                corridor_position_line.rotate(rotation).normalized();

            if (rotated_corridor_line.get_direction() == geometry::OrthogonalDirection::Right) {
                const geometry::OrthogonalLineGrid2D correct_position_line =
                    rotated_corridor_line + rotated_line.from;
                const geometry::Vector2Int dv = rotated_line.direction_vector();
                const int len = rotated_line.length();
                const geometry::Vector2Int to_ext{
                    correct_position_line.to.x + dv.x * len,
                    correct_position_line.to.y + dv.y * len,
                };
                const geometry::OrthogonalLineGrid2D correct_length_line(correct_position_line.from, to_ext);
                new_corridor_door_lines.push_back(
                    DoorLineGrid2D{.line = correct_length_line.rotate(-rotation), .length = corridor_door_line.length});
            } else if (rotated_corridor_line.get_direction() == geometry::OrthogonalDirection::Top) {
                for (const auto& corridor_position : rotated_corridor_line.grid_points_inclusive()) {
                    const geometry::OrthogonalLineGrid2D transformed_door_line = rotated_line + corridor_position;
                    new_corridor_door_lines.push_back(DoorLineGrid2D{
                        .line = transformed_door_line.rotate(-rotation), .length = corridor_door_line.length});
                }
            }
        }
    }

    return get_configuration_space(polygon, door_lines, fixed_polygon, new_corridor_door_lines);
}

ConfigurationSpaceGrid2D ConfigurationSpacesGenerator::get_configuration_space_over_corridors(
    const geometry::PolygonGrid2D& polygon, const std::vector<DoorLineGrid2D>& door_lines,
    const geometry::PolygonGrid2D& fixed_polygon, const std::vector<DoorLineGrid2D>& fixed_door_lines,
    const std::vector<std::pair<geometry::PolygonGrid2D, std::vector<DoorLineGrid2D>>>& corridors) {
    std::vector<geometry::OrthogonalLineGrid2D> configuration_space_lines;

    for (const auto& corridor_entry : corridors) {
        const ConfigurationSpaceGrid2D cs = get_configuration_space_over_corridor(
            polygon, door_lines, fixed_polygon, fixed_door_lines, corridor_entry.first, corridor_entry.second);
        configuration_space_lines.insert(configuration_space_lines.end(), cs.lines.begin(), cs.lines.end());
    }

    configuration_space_lines = geometry::OrthogonalLineIntersection::remove_intersections(configuration_space_lines);

    return ConfigurationSpaceGrid2D{std::move(configuration_space_lines)};
}

} // namespace edgar::generator::grid2d
