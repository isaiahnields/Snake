cmake_minimum_required(VERSION 3.16)
project(snake)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

if(ANDROID)
    find_package(SDL3 REQUIRED CONFIG)
else()
    include(FetchContent)

    set(SDL_SHARED OFF CACHE BOOL "Build SDL shared library" FORCE)
    set(SDL_STATIC ON CACHE BOOL "Build SDL static library" FORCE)

    FetchContent_Declare(
        SDL3
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG        release-3.4.18
        GIT_SHALLOW    TRUE
    )

    FetchContent_MakeAvailable(SDL3)
endif()

option(SNAKE_ENABLE_CLANG_TIDY "Run clang-tidy while compiling project targets" OFF)
if(SNAKE_ENABLE_CLANG_TIDY)
    find_program(CLANG_TIDY_EXECUTABLE NAMES clang-tidy)
    if(NOT CLANG_TIDY_EXECUTABLE)
        message(FATAL_ERROR "SNAKE_ENABLE_CLANG_TIDY is ON, but clang-tidy was not found")
    endif()
    set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXECUTABLE}")
endif()

add_subdirectory(src)

if(NOT ANDROID)
    enable_testing()
    add_subdirectory(tests)
endif()
