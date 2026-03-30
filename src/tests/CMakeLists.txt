find_package(GTest CONFIG REQUIRED)

include(GoogleTest)

add_executable(edgar_tests edgar_tests.cpp)
target_link_libraries(edgar_tests PRIVATE edgar GTest::gtest_main)
gtest_discover_tests(edgar_tests)

add_executable(edgar_parity_tests edgar_parity_tests.cpp)
target_link_libraries(edgar_parity_tests PRIVATE edgar GTest::gtest_main)
gtest_discover_tests(edgar_parity_tests)

find_package(yaml-cpp CONFIG REQUIRED)
add_executable(preset_loader_tests preset_loader_tests.cpp ../apps/main/preset_loader.cpp)
target_include_directories(preset_loader_tests PRIVATE "${CMAKE_SOURCE_DIR}/src/apps/main")
target_link_libraries(preset_loader_tests PRIVATE edgar yaml-cpp::yaml-cpp GTest::gtest_main)
gtest_discover_tests(preset_loader_tests)
