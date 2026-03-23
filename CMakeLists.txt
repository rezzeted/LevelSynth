cmake_minimum_required(VERSION 3.20)

project(ImguiPlayground VERSION 0.1.0 LANGUAGES CXX)

# Match static CRT (/MT) when vcpkg uses a *-static triplet (avoids LNK2038 with boost-graph, etc.).
if(POLICY CMP0091)
    cmake_policy(SET CMP0091 NEW)
endif()
if(MSVC AND DEFINED VCPKG_TARGET_TRIPLET AND VCPKG_TARGET_TRIPLET MATCHES "static")
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# Basic C++ settings
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Project structure
add_subdirectory(thirdparty)
add_subdirectory(src/libs)
add_subdirectory(src/apps)
enable_testing()
add_subdirectory(src/tests)

option(EDGAR_BUILD_DOCS "Build Doxygen HTML docs for edgar (requires doxygen)" OFF)
if(EDGAR_BUILD_DOCS)
    find_program(DOXYGEN_EXECUTABLE Doxygen REQUIRED)
    add_custom_target(edgar_docs
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_SOURCE_DIR}/docs/Doxyfile
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/docs
        COMMENT "Generating edgar Doxygen HTML (output under _build/docs/doxygen)"
        VERBATIM)
endif()
