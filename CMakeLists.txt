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

add_subdirectory(src)

if(NOT ANDROID)
    enable_testing()
    add_subdirectory(tests)
endif()
