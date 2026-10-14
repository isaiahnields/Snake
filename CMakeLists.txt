cmake_minimum_required(VERSION 3.16)
project(snake)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Add these flags BEFORE add_subdirectory(SDL) or in your FetchContent configuration
set(SDL_TESTS OFF CACHE BOOL "" FORCE)         # Disables building SDL test suite
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE) # Disables the test library helper
set(SDL_INSTALL_DOCS OFF CACHE BOOL "" FORCE) # Disables building documentation


find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
endif()

include(FetchContent)

if(ANDROID)
    find_package(SDL3 REQUIRED CONFIG)
else()
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

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.91.9b
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(imgui)

set(LEGACY_BUILD ON CACHE BOOL "" FORCE)
set(BUILD_ONLY cognito-idp CACHE STRING "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(AUTORUN_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)
set(AWS_SDK_WARNINGS_ARE_ERRORS OFF CACHE BOOL "" FORCE)
if(MINGW)
    set(USE_CPU_EXTENSIONS OFF CACHE BOOL "" FORCE)
endif()
FetchContent_Declare(
    aws_sdk_cpp
    GIT_REPOSITORY https://github.com/aws/aws-sdk-cpp.git
    GIT_TAG        1.11.911
    GIT_SHALLOW    TRUE
    GIT_SUBMODULES_RECURSE TRUE
)
FetchContent_MakeAvailable(aws_sdk_cpp)

find_package(Threads REQUIRED)
if(NOT WIN32 AND NOT ANDROID)
    find_package(CURL REQUIRED)
endif()

add_library(snake_imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer3.cpp
)
target_include_directories(snake_imgui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
target_link_libraries(snake_imgui PUBLIC SDL3::SDL3)
set_target_properties(snake_imgui PROPERTIES POSITION_INDEPENDENT_CODE ON)

option(SNAKE_ENABLE_CLANG_TIDY "Run clang-tidy while compiling project targets" OFF)
if(SNAKE_ENABLE_CLANG_TIDY)
    find_program(CLANG_TIDY_EXECUTABLE NAMES clang-tidy)
    if(NOT CLANG_TIDY_EXECUTABLE)
        message(FATAL_ERROR "SNAKE_ENABLE_CLANG_TIDY is ON, but clang-tidy was not found")
    endif()
    set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXECUTABLE}")
endif()

function(snake_copy_mingw_runtime target)
    if(MINGW AND WIN32)
        get_filename_component(compiler_directory "${CMAKE_CXX_COMPILER}" DIRECTORY)
        file(GLOB runtime_libraries CONFIGURE_DEPENDS
            "${compiler_directory}/libstdc++-6.dll"
            "${compiler_directory}/libgcc_s_*-1.dll"
            "${compiler_directory}/libwinpthread-1.dll"
        )
        if(NOT EXISTS "${compiler_directory}/libstdc++-6.dll")
            message(FATAL_ERROR "Cannot find the MinGW C++ runtime beside ${CMAKE_CXX_COMPILER}")
        endif()
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${runtime_libraries} "$<TARGET_FILE_DIR:${target}>"
            COMMENT "Copying compiler-matched MinGW runtime DLLs for ${target}"
            VERBATIM
        )
    endif()
endfunction()

add_subdirectory(src)

if(NOT ANDROID)
    enable_testing()
    add_subdirectory(tests)
endif()
