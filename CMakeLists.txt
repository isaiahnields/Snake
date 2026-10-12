cmake_minimum_required(VERSION 3.16)
project(snake)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

# 1. Bring in SDL3 (if you downloaded the .aar, use find_package instead)
if(ANDROID)
    find_package(SDL3 REQUIRED CONFIG)
else()
    add_subdirectory(vendored/SDL EXCLUDE_FROM_ALL)
endif()

# 2. Define your source files
set(GAME_SOURCES
    src/main.cpp
    src/snake_game.cpp
    src/neural_network.cpp
    src/dqn_agent.cpp
    src/renderer.cpp
)

# 3. Android needs a SHARED library named "main", Desktops need a WIN32 executable
if(ANDROID)
    add_library(main SHARED ${GAME_SOURCES})
else()
    add_executable(snake WIN32 ${GAME_SOURCES})
endif()

# 4. Link SDL3
if(ANDROID)
    target_link_libraries(main PRIVATE SDL3::SDL3 log)
else()
    target_link_libraries(snake PRIVATE SDL3::SDL3)
endif()
