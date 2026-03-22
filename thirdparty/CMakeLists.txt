# Third-party: SDL3, ImGui (via vcpkg manifest)

cmake_minimum_required(VERSION 3.20)

find_package(OpenGL REQUIRED)
find_package(SDL3 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

# Interface target keeps `main` linking unchanged (imgui + OpenGL)
add_library(imgui_impl INTERFACE)
target_link_libraries(imgui_impl INTERFACE imgui::imgui OpenGL::GL)
if(WIN32)
    target_link_libraries(imgui_impl INTERFACE opengl32)
endif()
