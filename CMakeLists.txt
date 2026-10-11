cmake_minimum_required(VERSION 3.16)
project(snake)

# set the output directory for built objects.
# This makes sure that the dynamic library goes into the build directory automatically.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

# This assumes the SDL source is available in vendored/SDL
add_subdirectory(vendored/SDL EXCLUDE_FROM_ALL)

# Create the game executable from its logical modules.
add_executable(snake WIN32
	src/main.cpp
	src/snake_game.cpp
	src/neural_network.cpp
	src/dqn_agent.cpp
	src/renderer.cpp
)

# Link to the actual SDL3 library.
target_link_libraries(snake PRIVATE SDL3::SDL3)