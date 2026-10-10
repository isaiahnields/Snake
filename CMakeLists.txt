cmake_minimum_required(VERSION 3.16)
project(Game
        VERSION 1.0.0 
        DESCRIPTION "A simple C++ project"
        LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_C_STANDARD 99)

# Option 1: Using FetchContent to pull bgfx.cmake directly (recommended for quick setups)
include(FetchContent)
FetchContent_Declare(
    bgfx.cmake
    GIT_REPOSITORY https://github.com/bkaradzic/bgfx.cmake.git
    GIT_TAG        master
)
FetchContent_MakeAvailable(bgfx.cmake)

# Define your executable
add_executable(bgfx_app main.cpp)

# Link bgfx (which automatically pulls in bx and bimg dependencies)
target_link_libraries(bgfx_app PRIVATE bgfx)