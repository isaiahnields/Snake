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

# 5. Tests (desktop only): one executable per source file under src/
if(NOT ANDROID)
    enable_testing()

    add_library(snake_core STATIC
        src/snake_game.cpp
        src/neural_network.cpp
        src/dqn_agent.cpp
        src/renderer.cpp
    )
    target_include_directories(snake_core PUBLIC src)
    target_link_libraries(snake_core PUBLIC SDL3::SDL3)

    foreach(name IN ITEMS snake_game neural_network dqn_agent renderer)
        add_executable(test_${name} EXCLUDE_FROM_ALL tests/test_${name}.cpp)
        target_link_libraries(test_${name} PRIVATE snake_core)
        add_test(NAME ${name} COMMAND test_${name})
    endforeach()
    set_tests_properties(renderer PROPERTIES ENVIRONMENT "SDL_VIDEO_DRIVER=dummy")

    add_custom_target(all_tests DEPENDS
        test_snake_game test_neural_network test_dqn_agent test_renderer)
endif()
