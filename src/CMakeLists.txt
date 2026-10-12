set(SNAKE_CORE_SOURCES
    snake_game.cpp
    neural_network.cpp
    dqn_agent.cpp
    renderer.cpp
)

add_library(snake_core ${SNAKE_CORE_SOURCES})
target_link_libraries(snake_core PUBLIC SDL3::SDL3)

if(ANDROID)
    add_library(main SHARED main.cpp)
    target_link_libraries(main PRIVATE snake_core log)
else()
    add_executable(snake WIN32 main.cpp)
    target_link_libraries(snake PRIVATE snake_core)
endif()
