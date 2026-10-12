set(SNAKE_CORE_SOURCES
    snake_game.cpp
    neural_network.cpp
    dqn_agent.cpp
    renderer.cpp
)

if(ANDROID)
    add_library(main SHARED main.cpp ${SNAKE_CORE_SOURCES})
    target_link_libraries(main PRIVATE SDL3::SDL3 log)
else()
    add_executable(snake WIN32 main.cpp ${SNAKE_CORE_SOURCES})
    target_link_libraries(snake PRIVATE SDL3::SDL3)
endif()
