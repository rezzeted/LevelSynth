#pragma once

// Derived from Edgar-DotNet `DoorUtils.MergeDoorLines` (MIT).

#include "edgar/generator/grid2d/door_line_grid2d.hpp"
#include "edgar/geometry/orthogonal_line_grid2d.hpp"

#include <algorithm>
#include <map>
#include <stdexcept>
#include <vector>

namespace edgar::generator::grid2d {

inline std::vector<DoorLineGrid2D> merge_door_lines(std::vector<DoorLineGrid2D> door_lines) {
    std::map<geometry::OrthogonalDirection, std::vector<DoorLineGrid2D>> by_dir;
    for (auto& d : door_lines) {
        by_dir[d.line.get_direction()].push_back(std::move(d));
    }

    std::vector<DoorLineGrid2D> result;

    for (auto& group : by_dir) {
        if (group.first == geometry::OrthogonalDirection::Undefined) {
            throw std::invalid_argument("merge_door_lines: undefined door direction");
        }
        std::vector<DoorLineGrid2D> same = std::move(group.second);
        while (!same.empty()) {
            DoorLineGrid2D door_line = std::move(same.back());
            same.pop_back();
            bool found = true;
            while (found) {
                found = false;
                for (auto it = same.begin(); it != same.end();) {
                    if (it->length != door_line.length) {
                        ++it;
                        continue;
                    }
                    const geometry::Vector2Int dv = door_line.line.direction_vector();
                    if (door_line.line.to + dv == it->line.from) {
                        door_line.line = geometry::OrthogonalLineGrid2D(door_line.line.from, it->line.to);
                        it = same.erase(it);
                        found = true;
                    } else if (door_line.line.from - dv == it->line.to) {
                        door_line.line = geometry::OrthogonalLineGrid2D(it->line.from, door_line.line.to);
                        it = same.erase(it);
                        found = true;
                    } else {
                        ++it;
                    }
                }
            }
            result.push_back(std::move(door_line));
        }
    }
    return result;
}

} // namespace edgar::generator::grid2d
