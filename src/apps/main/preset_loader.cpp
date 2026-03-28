#include "preset_loader.hpp"

#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <fstream>
#include <sstream>

namespace edgar::generator::grid2d {

static PresetRoomSet::RoomEntry parse_room_entry(const std::string& name, const YAML::Node& node,
                                                  int default_door_length, int default_corner_distance) {
    PresetRoomSet::RoomEntry entry;
    entry.name = name;

    const auto& shape_node = node["shape"];
    if (shape_node && shape_node.IsSequence()) {
        for (const auto& pt : shape_node) {
            entry.shape.push_back(edgar::geometry::Vector2Int{pt[0].as<int>(), pt[1].as<int>()});
        }
    }

    const auto& dm = node["doorMode"];
    if (dm) {
        if (dm["doorLength"]) {
            entry.door_length = dm["doorLength"].as<int>();
        } else {
            entry.door_length = default_door_length;
        }
        if (dm["cornerDistance"]) {
            entry.corner_distance = dm["cornerDistance"].as<int>();
        } else {
            entry.corner_distance = default_corner_distance;
        }
        entry.door_mode = "OverlapMode";
        if (dm["doorPositions"]) {
            entry.door_mode = "SpecificPositionsMode";
            for (const auto& dp : dm["doorPositions"]) {
                entry.specific_doors.push_back({
                    edgar::geometry::Vector2Int{dp[0][0].as<int>(), dp[0][1].as<int>()},
                    edgar::geometry::Vector2Int{dp[1][0].as<int>(), dp[1][1].as<int>()}
                });
            }
        }
    } else {
        entry.door_length = default_door_length;
        entry.corner_distance = default_corner_distance;
        entry.door_mode = "OverlapMode";
    }

    return entry;
}

static PresetRoomSet load_room_set(const std::string& path) {
    PresetRoomSet set;
    std::ifstream f(path);
    if (!f.is_open()) {
        return set;
    }

    YAML::Node root = YAML::Load(f);
    set.name = root["name"] ? root["name"].as<std::string>() : "";

    int def_dl = 1, def_cd = 1;
    if (const auto& def = root["default"]) {
        set.has_defaults = true;
        if (def["doorMode"]) {
            if (def["doorMode"]["doorLength"]) {
                def_dl = def["doorMode"]["doorLength"].as<int>();
            }
            if (def["doorMode"]["cornerDistance"]) {
                def_cd = def["doorMode"]["cornerDistance"].as<int>();
            }
        }
    }
    set.default_door_length = def_dl;
    set.default_corner_distance = def_cd;

    if (const auto& rds = root["roomDescriptions"]) {
        for (const auto& kv : rds) {
            set.rooms.push_back(parse_room_entry(kv.first.as<std::string>(), kv.second, def_dl, def_cd));
        }
    }

    return set;
}

PresetCatalog load_preset_catalog(const std::string& base_path) {
    PresetCatalog catalog;
    catalog.base_path = base_path;

    const std::string rooms_dir = base_path + "/Rooms";
    for (const auto& entry : std::filesystem::directory_iterator(rooms_dir)) {
        if (entry.path().extension() == ".yml") {
            auto rs = load_room_set(entry.path().string());
            if (!rs.rooms.empty()) {
                catalog.room_sets.push_back(std::move(rs));
            }
        }
    }

    const std::string maps_dir = base_path + "/Maps";
    for (const auto& entry : std::filesystem::directory_iterator(maps_dir)) {
        if (entry.path().extension() != ".yml") {
            continue;
        }
        std::ifstream f(entry.path().string());
        if (!f.is_open()) {
            continue;
        }
        YAML::Node root = YAML::Load(f);

        PresetMap map;
        map.filename = entry.path().filename().string();
        map.display_name = entry.path().stem().string();

        if (root["roomsRange"]) {
            map.room_from = root["roomsRange"]["from"].as<int>();
            map.room_to = root["roomsRange"]["to"].as<int>();
        }

        if (root["passages"]) {
            for (const auto& p : root["passages"]) {
                map.passages.push_back({p[0].as<int>(), p[1].as<int>()});
            }
        }

        if (root["rooms"]) {
            for (const auto& kv : root["rooms"]) {
                PresetMap::RoomOverride ov;
                const auto ids = kv.first.as<std::string>();
                std::stringstream ss(ids);
                std::string token;
                while (std::getline(ss, token, ',')) {
                    int val = 0;
                    bool neg = false;
                    std::string trimmed;
                    for (char c : token) {
                        if (c == '-') {
                            neg = true;
                        } else if (c == ' ' || c == '[' || c == ']') {
                        } else {
                            trimmed += c;
                        }
                    }
                    for (char c : trimmed) {
                        val = val * 10 + (c - '0');
                    }
                    if (neg) val = -val;
                    ov.room_ids.push_back(val);
                }
                const auto& shapes = kv.second["roomShapes"];
                if (shapes) {
                    for (const auto& s : shapes) {
                        if (s.IsMap()) {
                            if (s["roomDescriptionName"]) {
                                ov.room_description_names.push_back(s["roomDescriptionName"].as<std::string>());
                            }
                        }
                    }
                }
                map.room_overrides.push_back(std::move(ov));
            }
        }

        if (root["defaultRoomShapes"]) {
            for (const auto& s : root["defaultRoomShapes"]) {
                PresetMap::DefaultRoomShapes drs;
                if (s.IsMap()) {
                    drs.set_name = s["setName"].as<std::string>();
                    if (s["roomDescriptionName"]) {
                        drs.room_description_name = s["roomDescriptionName"].as<std::string>();
                    }
                    if (s["scale"]) {
                        for (const auto& v : s["scale"]) {
                            drs.scale.push_back(v.as<int>());
                        }
                    }
                } else if (s.IsScalar()) {
                    drs.set_name = s.as<std::string>();
                }
                map.default_room_shapes.push_back(std::move(drs));
            }
        }

        if (root["customRoomDescriptionsSet"]) {
            map.has_custom_descriptions = true;
            const auto& crd = root["customRoomDescriptionsSet"];
            int cdl = 1, ccd = 1;
            if (const auto& def = crd["default"]) {
                if (def["doorMode"]) {
                    if (def["doorMode"]["doorLength"]) cdl = def["doorMode"]["doorLength"].as<int>();
                    if (def["doorMode"]["cornerDistance"]) ccd = def["doorMode"]["cornerDistance"].as<int>();
                }
            }
            map.custom_descriptions.default_door_length = cdl;
            map.custom_descriptions.default_corner_distance = ccd;
            map.custom_descriptions.has_defaults = true;
            if (const auto& rds = crd["roomDescriptions"]) {
                for (const auto& kv : rds) {
                    map.custom_descriptions.rooms.push_back(
                        parse_room_entry(kv.first.as<std::string>(), kv.second, cdl, ccd));
                }
            }
        }

        if (root["corridors"]) {
            const auto& corr = root["corridors"];
            map.corridors_enabled = corr["enable"] ? corr["enable"].as<bool>() : false;
            if (corr["offsets"]) {
                for (const auto& o : corr["offsets"]) {
                    map.corridor_offsets.push_back(o.as<int>());
                }
            }
            if (corr["corridorShapes"]) {
                for (const auto& cs : corr["corridorShapes"]) {
                    PresetMap::DefaultRoomShapes drs;
                    if (cs.IsMap()) {
                        drs.set_name = cs["setName"].as<std::string>();
                    } else if (cs.IsScalar()) {
                        drs.set_name = cs.as<std::string>();
                    }
                    map.corridor_shapes.push_back(std::move(drs));
                }
            }
        }

        catalog.maps.push_back(std::move(map));
    }

    std::sort(catalog.maps.begin(), catalog.maps.end(),
              [](const PresetMap& a, const PresetMap& b) { return a.display_name < b.display_name; });
    std::sort(catalog.room_sets.begin(), catalog.room_sets.end(),
              [](const PresetRoomSet& a, const PresetRoomSet& b) { return a.name < b.name; });

    return catalog;
}

static const PresetRoomSet::RoomEntry* find_room_entry(
    const std::vector<PresetRoomSet>& sets, const std::string& set_name, const std::string& entry_name) {
    for (const auto& rs : sets) {
        if (rs.name == set_name) {
            for (const auto& r : rs.rooms) {
                if (r.name == entry_name) {
                    return &r;
                }
            }
        }
    }
    return nullptr;
}

static RoomTemplateGrid2D build_room_template(const PresetRoomSet::RoomEntry& entry) {
    std::vector<edgar::geometry::Vector2Int> pts = entry.shape;
    edgar::geometry::PolygonGrid2D poly(pts);

    auto door_mode = std::make_shared<SimpleDoorModeGrid2D>(entry.door_length, entry.corner_distance);
    return RoomTemplateGrid2D(std::move(poly), std::move(door_mode), entry.name);
}

LevelDescriptionGrid2D<int> build_level_from_preset(
    const PresetMap& map, const std::vector<PresetRoomSet>& room_sets) {

    LevelDescriptionGrid2D<int> level;

    std::unordered_map<int, std::vector<RoomTemplateGrid2D>> room_templates;

    for (const auto& ov : map.room_overrides) {
        std::vector<RoomTemplateGrid2D> templates;
        for (const auto& rn : ov.room_description_names) {
            const PresetRoomSet::RoomEntry* found = nullptr;
            if (map.has_custom_descriptions) {
                for (const auto& r : map.custom_descriptions.rooms) {
                    if (r.name == rn) {
                        found = &r;
                        break;
                    }
                }
            }
            if (!found) {
                for (const auto& drs : map.default_room_shapes) {
                    found = find_room_entry(room_sets, drs.set_name, rn);
                    if (found) break;
                }
            }
            if (found) {
                templates.push_back(build_room_template(*found));
            }
        }
        for (int id : ov.room_ids) {
            room_templates[id] = templates;
        }
    }

    for (const auto& drs : map.default_room_shapes) {
        if (!drs.room_description_name.empty()) {
            continue;
        }
        const PresetRoomSet* matching_set = nullptr;
        for (const auto& rs : room_sets) {
            if (rs.name == drs.set_name) {
                matching_set = &rs;
                break;
            }
        }
        if (!matching_set) {
            continue;
        }
        for (int id = map.room_from; id <= map.room_to; ++id) {
            if (room_templates.count(id)) {
                continue;
            }
            std::vector<RoomTemplateGrid2D> templates;
            for (const auto& r : matching_set->rooms) {
                templates.push_back(build_room_template(r));
            }
            room_templates[id] = std::move(templates);
        }
    }

    for (int id = map.room_from; id <= map.room_to; ++id) {
        auto it = room_templates.find(id);
        if (it != room_templates.end() && !it->second.empty()) {
            RoomDescriptionGrid2D desc(false, it->second);
            level.add_room(id, desc);
        } else {
            auto square = RoomTemplateGrid2D(edgar::geometry::PolygonGrid2D::get_square(8),
                                             std::make_shared<SimpleDoorModeGrid2D>(1, 1));
            RoomDescriptionGrid2D desc(false, {square});
            level.add_room(id, desc);
        }
    }

    for (const auto& p : map.passages) {
        level.add_connection(p.a, p.b);
    }

    return level;
}

LevelDescriptionGrid2D<int> build_level_from_preset(const PresetMap& map, const PresetCatalog& catalog) {
    return build_level_from_preset(map, catalog.room_sets);
}

} // namespace edgar::generator::grid2d
