cmake_minimum_required(VERSION 3.12)
project(MatchingEngine VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set build type to Release by default
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler flags
if(MSVC)
    add_compile_options(/W4 /WX-)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")
endif()

# Include directories
include_directories(${PROJECT_SOURCE_DIR}/include)

# Core library sources
set(CORE_SOURCES
    src/Order.cpp
    src/Trade.cpp
    src/OrderBook.cpp
    src/MatchingEngine.cpp
)

# Create core library
add_library(matching_engine_core STATIC ${CORE_SOURCES})
target_include_directories(matching_engine_core PUBLIC 
    ${PROJECT_SOURCE_DIR}/include
)

# Server executable
add_executable(matching_server
    src/main_server.cpp
    src/Server.cpp
)

target_link_libraries(matching_server PRIVATE matching_engine_core)

# Client executable
add_executable(matching_client
    src/main_client.cpp
    src/Client.cpp
)

target_link_libraries(matching_client PRIVATE matching_engine_core)

# Platform-specific libraries
if(WIN32)
    target_link_libraries(matching_server PRIVATE ws2_32)
    target_link_libraries(matching_client PRIVATE ws2_32)
else()
    # Link pthread for Linux/Unix
    find_package(Threads REQUIRED)
    target_link_libraries(matching_server PRIVATE Threads::Threads)
    target_link_libraries(matching_client PRIVATE Threads::Threads)
endif()

# Installation
install(TARGETS matching_server matching_client matching_engine_core
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(DIRECTORY include/ DESTINATION include)

# Tests
option(BUILD_TESTS "Build tests" ON)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Print configuration
message(STATUS "")
message(STATUS "========================================")
message(STATUS "Matching Engine Configuration")
message(STATUS "========================================")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "Build tests: ${BUILD_TESTS}")
message(STATUS "========================================")
message(STATUS "")

