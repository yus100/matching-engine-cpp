cmake_minimum_required(VERSION 3.12)

# Download and configure Google Test
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Enable testing
enable_testing()

# Include directories
include_directories(${PROJECT_SOURCE_DIR}/include)

# Test executables
set(TEST_SOURCES
    test_order.cpp
    test_orderbook.cpp
    test_matching_engine.cpp
    test_integration.cpp
)

# Create test executable
add_executable(matching_engine_tests ${TEST_SOURCES})

# Link against gtest, gtest_main, and our core library
target_link_libraries(matching_engine_tests
    PRIVATE
    GTest::gtest_main
    GTest::gmock
    matching_engine_core
)

# Add tests to CTest
include(GoogleTest)
gtest_discover_tests(matching_engine_tests)

# Custom target to run tests
add_custom_target(run_tests
    COMMAND matching_engine_tests
    DEPENDS matching_engine_tests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

message(STATUS "Tests configured successfully")

