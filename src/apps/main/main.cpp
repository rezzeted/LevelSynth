#define SDL_MAIN_HANDLED

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_opengl3.h"
#include "layout_preview.hpp"
#include "preset_loader.hpp"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_opengl.h>
#include <algorithm>
#include <cstdio>
#include <exception>
#include <random>
#include <vector>

#include "edgar/edgar.hpp"
#include "edgar/generator/grid2d/layout_door_computation.hpp"
#include "edgar/io/layout_json.hpp"

namespace {

edgar::generator::grid2d::PresetCatalog g_catalog;
bool g_catalog_loaded = false;
int g_selected_preset = 0;

edgar::generator::grid2d::GraphBasedGeneratorConfiguration g_gen_config;

char g_edgar_log[512] = "Select a preset and click Generate.";
std::vector<edgar::generator::grid2d::LayoutGrid2D<int>> g_layouts;
int g_layout_index = 0;
bool g_use_random_seed = false;
int g_seed = 42;
int g_num_layouts = 1;
double g_last_time_ms = 0.0;
int g_last_iterations = 0;
int g_last_rooms = 0;

bool g_compute_doors = true;
bool g_export_pending = false;

void generate_from_preset(int preset_idx, unsigned rng_seed) {
    using namespace edgar::generator::grid2d;

    g_layouts.clear();
    g_layout_index = 0;

    const auto& map = g_catalog.maps[static_cast<std::size_t>(preset_idx)];
    LevelDescriptionGrid2D<int> level = build_level_from_preset(map, g_catalog);

    GraphBasedGeneratorGrid2D<int> generator(level, g_gen_config);
    std::mt19937 rng(rng_seed);
    generator.inject_random_generator(std::move(rng));

    const int n = std::clamp(g_num_layouts, 1, 64);
    g_layouts.reserve(static_cast<size_t>(n));
    for (int i = 0; i < n; ++i) {
        auto layout = generator.generate_layout();
        if (g_compute_doors) {
            std::mt19937 door_rng(rng_seed + static_cast<unsigned>(i));
            auto graph = level.get_graph();
            compute_layout_doors(layout, level, graph, door_rng);
        }
        g_layouts.push_back(std::move(layout));
        g_last_rooms = static_cast<int>(g_layouts.back().rooms.size());
        g_last_time_ms = generator.time_total_ms();
        g_last_iterations = generator.iterations_count();
    }

    int total_doors = 0;
    for (const auto& room : g_layouts[0].rooms) {
        total_doors += static_cast<int>(room.doors.size());
    }

    std::snprintf(g_edgar_log, sizeof(g_edgar_log),
                  "Generated %d layout(s) from '%s' | rooms=%d  doors=%d  time=%.2f ms  iters=%d",
                  n, map.display_name.c_str(), g_last_rooms, total_doors,
                  g_last_time_ms, g_last_iterations);
}

void generate_hardcoded(unsigned rng_seed) {
    using namespace edgar;
    using namespace edgar::generator::grid2d;

    g_layouts.clear();
    g_layout_index = 0;

    auto square = RoomTemplateGrid2D(geometry::PolygonGrid2D::get_square(8),
                                     std::make_shared<SimpleDoorModeGrid2D>(1, 1));
    auto rectangle = RoomTemplateGrid2D(geometry::PolygonGrid2D::get_rectangle(6, 10),
                                        std::make_shared<SimpleDoorModeGrid2D>(1, 1));
    RoomDescriptionGrid2D room_desc(false, {square, rectangle});
    LevelDescriptionGrid2D<int> level;
    level.add_room(0, room_desc);
    level.add_room(1, room_desc);
    level.add_room(2, room_desc);
    level.add_room(3, room_desc);
    level.add_connection(0, 1);
    level.add_connection(0, 3);
    level.add_connection(1, 2);
    level.add_connection(2, 3);

    GraphBasedGeneratorGrid2D<int> generator(level, g_gen_config);
    std::mt19937 rng(rng_seed);
    generator.inject_random_generator(std::move(rng));

    const int n = std::clamp(g_num_layouts, 1, 64);
    g_layouts.reserve(static_cast<size_t>(n));
    for (int i = 0; i < n; ++i) {
        auto layout = generator.generate_layout();
        if (g_compute_doors) {
            std::mt19937 door_rng(rng_seed + static_cast<unsigned>(i));
            auto graph = level.get_graph();
            compute_layout_doors(layout, level, graph, door_rng);
        }
        g_layouts.push_back(std::move(layout));
        g_last_rooms = static_cast<int>(g_layouts.back().rooms.size());
        g_last_time_ms = generator.time_total_ms();
        g_last_iterations = generator.iterations_count();
    }

    std::snprintf(g_edgar_log, sizeof(g_edgar_log),
                  "Generated %d layout(s) (4-room cycle) | rooms=%d  time=%.2f ms  iters=%d",
                  n, g_last_rooms, g_last_time_ms, g_last_iterations);
}

std::string get_executable_dir() {
    char path[1024] = {};
#ifdef _WIN32
    GetModuleFileNameA(nullptr, path, sizeof(path));
    char* last_slash = strrchr(path, '\\');
    if (last_slash) *last_slash = '\0';
#else
    auto len = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (len > 0) {
        path[len] = '\0';
        char* last_slash = strrchr(path, '/');
        if (last_slash) *last_slash = '\0';
    }
#endif
    return std::string(path);
}

} // namespace

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    SDL_SetMainReady();
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        (void)fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    const int window_width = 1280;
    const int window_height = 720;
    const SDL_WindowFlags window_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
        | SDL_WINDOW_HIGH_PIXEL_DENSITY;
    SDL_Window* window = SDL_CreateWindow("LevelSynth", window_width, window_height, window_flags);
    if (!window) {
        (void)fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    if (!gl_context) {
        (void)fprintf(stderr, "SDL_GL_CreateContext failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleFonts;
    io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleViewports;

    ImGui::StyleColorsDark();

    ImGuiIO& io_ref = ImGui::GetIO();
    ImFontConfig font_cfg;
    font_cfg.OversampleH = 2;
    font_cfg.OversampleV = 2;
    font_cfg.PixelSnapH = true;
    const float font_size_px = 19.0f;
#ifdef _WIN32
    const char* font_paths[] = {
        "C:\\Windows\\Fonts\\segoeui.ttf",
        "C:\\Windows\\Fonts\\seguiui.ttf",
        "C:\\Windows\\Fonts\\arial.ttf",
    };
    for (const char* path : font_paths) {
        if (io_ref.Fonts->AddFontFromFileTTF(path, font_size_px, &font_cfg) != nullptr)
            break;
    }
#else
    const char* font_paths[] = {
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        "/usr/share/fonts/TTF/DejaVuSans.ttf",
    };
    for (const char* path : font_paths) {
        if (io_ref.Fonts->AddFontFromFileTTF(path, font_size_px, &font_cfg) != nullptr)
            break;
    }
#endif

    ImGui_ImplSDL3_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init("#version 130");

    {
        std::string base = get_executable_dir() + "/resources/edgar_gui";
        try {
            g_catalog = edgar::generator::grid2d::load_preset_catalog(base);
            g_catalog_loaded = !g_catalog.maps.empty();
        } catch (...) {
            g_catalog_loaded = false;
        }
    }

    constexpr float k_settings_panel_w = 360.0f;

    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT)
                running = false;
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED &&
                event.window.windowID == SDL_GetWindowID(window))
                running = false;
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);

        if (ImGui::Begin("LevelSynth", nullptr,
                         ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove
                             | ImGuiWindowFlags_NoBringToFrontOnFocus)) {

            ImGui::BeginChild("##settings", ImVec2(k_settings_panel_w, 0.0f), true);

            ImGui::SeparatorText("Preset");
            if (g_catalog_loaded) {
                const int n_maps = static_cast<int>(g_catalog.maps.size());
                g_selected_preset = std::clamp(g_selected_preset, 0, n_maps - 1);
                const auto& cur = g_catalog.maps[static_cast<std::size_t>(g_selected_preset)];

                if (ImGui::BeginCombo("Map", cur.display_name.c_str())) {
                    for (int i = 0; i < n_maps; ++i) {
                        const bool selected = (i == g_selected_preset);
                        if (ImGui::Selectable(g_catalog.maps[static_cast<std::size_t>(i)].display_name.c_str(), selected)) {
                            g_selected_preset = i;
                        }
                        if (selected) ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }

                ImGui::Text("Rooms: %d-%d | Passages: %d",
                    cur.room_from, cur.room_to, static_cast<int>(cur.passages.size()));
                ImGui::Text("Corridors: %s", cur.corridors_enabled ? "Yes" : "No");
            } else {
                ImGui::TextUnformatted("No presets found (resources/edgar_gui)");
                ImGui::TextUnformatted("Using built-in 4-room cycle");
            }
            ImGui::Spacing();

            ImGui::SeparatorText("Generation");
            ImGui::Checkbox("Random seed", &g_use_random_seed);
            if (!g_use_random_seed) {
                ImGui::InputInt("Seed", &g_seed);
            }
            ImGui::InputInt("Layouts", &g_num_layouts);
            g_num_layouts = std::clamp(g_num_layouts, 1, 64);
            ImGui::Checkbox("Compute doors", &g_compute_doors);
            ImGui::Spacing();

            auto& sa = g_gen_config.simulated_annealing;
            ImGui::SeparatorText("SA Configuration");
            ImGui::InputInt("Cycles", &sa.cycles);
            sa.cycles = std::max(sa.cycles, 1);
            ImGui::InputInt("Trials/cycle", &sa.trials_per_cycle);
            sa.trials_per_cycle = std::max(sa.trials_per_cycle, 1);
            ImGui::InputInt("Max iters w/o success", &sa.max_iterations_without_success);
            sa.max_iterations_without_success = std::max(sa.max_iterations_without_success, 1);
            ImGui::InputInt("Max stage2 failures", &sa.max_stage_two_failures);
            sa.max_stage_two_failures = std::max(sa.max_stage_two_failures, 1);
            ImGui::InputInt("Max perturb radius", &sa.max_perturbation_radius);
            sa.max_perturbation_radius = std::max(sa.max_perturbation_radius, 1);
            ImGui::Checkbox("Handle trees greedily", &sa.handle_trees_greedily);

            if (ImGui::Button("Generate", ImVec2(-1.0f, 0.0f))) {
                try {
                    unsigned seed = 0;
                    if (g_use_random_seed) {
                        std::random_device rd;
                        seed = rd();
                    } else {
                        seed = static_cast<unsigned>(g_seed);
                    }

                    if (g_catalog_loaded) {
                        generate_from_preset(g_selected_preset, seed);
                    } else {
                        generate_hardcoded(seed);
                    }
                } catch (const std::exception& e) {
                    std::snprintf(g_edgar_log, sizeof(g_edgar_log), "Error: %s", e.what());
                    g_layouts.clear();
                    g_layout_index = 0;
                    g_last_rooms = -1;
                }
            }

            ImGui::Spacing();
            if (!g_layouts.empty()) {
                if (ImGui::Button("Export JSON", ImVec2(-1.0f, 0.0f))) {
                    g_export_pending = true;
                }
            }

            ImGui::EndChild();

            ImGui::SameLine();

            ImGui::BeginChild("##canvas", ImVec2(0.0f, 0.0f), true);
            ImGui::SeparatorText("Generator");
            ImGui::TextWrapped("%s", g_edgar_log);

            if (g_layouts.size() > 1) {
                const int max_i = static_cast<int>(g_layouts.size()) - 1;
                g_layout_index = std::clamp(g_layout_index, 0, max_i);
                ImGui::SliderInt("Shown layout", &g_layout_index, 0, max_i);
            }

            if (!g_layouts.empty() && g_layout_index >= 0
                && g_layout_index < static_cast<int>(g_layouts.size())
                && !g_layouts[static_cast<size_t>(g_layout_index)].rooms.empty()) {

                const auto& shown = g_layouts[static_cast<size_t>(g_layout_index)];

                ImGui::Separator();
                int total_doors = 0;
                for (const auto& room : shown.rooms) {
                    total_doors += static_cast<int>(room.doors.size());
                }
                ImGui::Text("Layout %d/%d  |  rooms=%d  doors=%d",
                    g_layout_index + 1, static_cast<int>(g_layouts.size()),
                    static_cast<int>(shown.rooms.size()), total_doors);

                ImGui::TextUnformatted("Layout preview (corridors in blue):");
                draw_layout_preview_imgui(shown);
            }
            ImGui::EndChild();
        }
        ImGui::End();

        if (g_export_pending && !g_layouts.empty()) {
            g_export_pending = false;
            try {
                const auto& layout = g_layouts[static_cast<size_t>(g_layout_index)];
                auto j = edgar::io::layout_to_json(layout);
                std::string json_str = j.dump(2);
                std::string filename = "layout_export.json";
                auto f = fopen(filename.c_str(), "w");
                if (f) {
                    fwrite(json_str.data(), 1, json_str.size(), f);
                    fclose(f);
                    std::snprintf(g_edgar_log, sizeof(g_edgar_log),
                                  "Exported layout to %s (%d rooms)", filename.c_str(),
                                  static_cast<int>(layout.rooms.size()));
                }
            } catch (const std::exception& e) {
                std::snprintf(g_edgar_log, sizeof(g_edgar_log), "Export error: %s", e.what());
            }
        }

        ImGui::Render();
        SDL_GL_MakeCurrent(window, gl_context);
        const int fb_w = (int)(io.DisplaySize.x * io.DisplayFramebufferScale.x);
        const int fb_h = (int)(io.DisplaySize.y * io.DisplayFramebufferScale.y);
        glViewport(0, 0, fb_w, fb_h);
        glClearColor(0.1f, 0.1f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DestroyContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
