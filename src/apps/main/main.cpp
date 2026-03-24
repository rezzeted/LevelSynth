// Minimal ImGui + SDL3 + OpenGL3 application (C++20)
// Static link: use our main(), tell SDL we handle entry point
#define SDL_MAIN_HANDLED

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_opengl3.h"
#include "layout_preview.hpp"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_opengl.h>
#include <algorithm>
#include <cstdio>
#include <exception>
#include <random>
#include <vector>

#include "edgar/edgar.hpp"

namespace {

void run_generate_preset_cycle(int num_layouts, unsigned rng_seed, char* edgar_log, size_t log_cap,
                               std::vector<edgar::generator::grid2d::LayoutGrid2D<int>>& out_layouts,
                               int& out_last_rooms, double& out_last_time_ms, int& out_last_iterations) {
    using namespace edgar;
    using namespace edgar::generator;
    using namespace edgar::generator::grid2d;

    out_layouts.clear();

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

    GraphBasedGeneratorGrid2D<int> generator(level);
    std::mt19937 rng(rng_seed);
    generator.inject_random_generator(std::move(rng));

    const int n = std::clamp(num_layouts, 1, 64);
    out_layouts.reserve(static_cast<size_t>(n));
    for (int i = 0; i < n; ++i) {
        const auto layout = generator.generate_layout();
        out_layouts.push_back(layout);
        out_last_rooms = static_cast<int>(layout.rooms.size());
        out_last_time_ms = generator.time_total_ms();
        out_last_iterations = generator.iterations_count();
    }

    std::snprintf(edgar_log, log_cap,
                  "layouts=%d  last: rooms=%d  time=%.2f ms  iterations=%d", n, out_last_rooms,
                  out_last_time_ms, out_last_iterations);
}

} // namespace

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    SDL_SetMainReady();
    // SDL3: SDL_Init returns true on success, false on failure
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
        | SDL_WINDOW_HIGH_PIXEL_DENSITY;  // HiDPI: request native pixel density back buffer
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
    // HiDPI: scale fonts and viewports from DPI (backend sets DisplayFramebufferScale)
    io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleFonts;
    io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleViewports;

    ImGui::StyleColorsDark();

    // Load a nice TTF font (replaces the default pixel font for crisp text)
    ImGuiIO& io_ref = ImGui::GetIO();
    ImFontConfig font_cfg;
    font_cfg.OversampleH = 2;
    font_cfg.OversampleV = 2;
    font_cfg.PixelSnapH = true;
    const float font_size_px = 19.0f;
#ifdef _WIN32
    const char* font_paths[] = {
        "C:\\Windows\\Fonts\\segoeui.ttf",   // Segoe UI
        "C:\\Windows\\Fonts\\seguiui.ttf",   // Segoe UI (legacy name)
        "C:\\Windows\\Fonts\\arial.ttf",
    };
    bool font_loaded = false;
    for (const char* path : font_paths) {
        if (io_ref.Fonts->AddFontFromFileTTF(path, font_size_px, &font_cfg) != nullptr) {
            font_loaded = true;
            break;
        }
    }
    (void)font_loaded;
#else
    // Linux: try common system font paths
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

    static char edgar_log[512] = "Click Generate to run the graph-based generator (4-room cycle preset).";
    static std::vector<edgar::generator::grid2d::LayoutGrid2D<int>> edgar_layouts;
    static int edgar_layout_index = 0;
    static bool use_random_seed = false;
    static int generator_seed = 42;
    static int number_of_layouts = 1;
    static double last_time_ms = 0.0;
    static int last_iterations = 0;
    static int last_rooms = 0;

    constexpr float k_settings_panel_w = 340.0f;

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
            ImGui::SeparatorText("Main settings");
            ImGui::TextUnformatted("Preset: 4-room cycle");
            ImGui::Spacing();

            ImGui::Checkbox("Random seed", &use_random_seed);
            if (!use_random_seed) {
                ImGui::InputInt("Seed", &generator_seed);
            }
            ImGui::InputInt("Number of layouts", &number_of_layouts);
            number_of_layouts = std::clamp(number_of_layouts, 1, 64);

            if (ImGui::Button("Generate", ImVec2(-1.0f, 0.0f))) {
                try {
                    unsigned seed = 0;
                    if (use_random_seed) {
                        std::random_device rd;
                        seed = rd();
                    } else {
                        seed = static_cast<unsigned>(generator_seed);
                    }

                    run_generate_preset_cycle(number_of_layouts, seed, edgar_log, sizeof edgar_log, edgar_layouts,
                                              last_rooms, last_time_ms, last_iterations);
                    edgar_layout_index = 0;
                } catch (const std::exception& e) {
                    std::snprintf(edgar_log, sizeof edgar_log, "Error: %s", e.what());
                    edgar_layouts.clear();
                    edgar_layout_index = 0;
                    last_rooms = -1;
                }
            }
            ImGui::EndChild();

            ImGui::SameLine();

            ImGui::BeginChild("##canvas", ImVec2(0.0f, 0.0f), true);
            ImGui::SeparatorText("Generator");
            ImGui::TextWrapped("%s", edgar_log);
            if (last_rooms >= 0 && !edgar_layouts.empty()) {
                ImGui::Text("Last: time=%.2f ms  iterations=%d  rooms=%d", last_time_ms, last_iterations,
                            last_rooms);
            }

            if (edgar_layouts.size() > 1) {
                const int max_i = static_cast<int>(edgar_layouts.size()) - 1;
                edgar_layout_index = std::clamp(edgar_layout_index, 0, max_i);
                ImGui::SliderInt("Shown layout", &edgar_layout_index, 0, max_i);
            }

            if (!edgar_layouts.empty() && edgar_layout_index >= 0
                && edgar_layout_index < static_cast<int>(edgar_layouts.size())
                && !edgar_layouts[static_cast<size_t>(edgar_layout_index)].rooms.empty()) {
                ImGui::Separator();
                ImGui::TextUnformatted("Layout preview (orthogonal outlines, corridors highlighted):");
                draw_layout_preview_imgui(edgar_layouts[static_cast<size_t>(edgar_layout_index)]);
            }
            ImGui::EndChild();
        }
        ImGui::End();

        ImGui::Render();
        SDL_GL_MakeCurrent(window, gl_context);
        // HiDPI: use framebuffer size in pixels, not logical DisplaySize
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
