find_package(GTest CONFIG REQUIRED)

include(GoogleTest)

add_executable(edgar_tests edgar_tests.cpp)
target_link_libraries(edgar_tests PRIVATE edgar GTest::gtest_main)
gtest_discover_tests(edgar_tests)

add_executable(edgar_parity_tests edgar_parity_tests.cpp)
target_link_libraries(edgar_parity_tests PRIVATE edgar GTest::gtest_main)
gtest_discover_tests(edgar_parity_tests)
