set(SNAKE_CORE_SOURCES
    snake_game.cpp
    renderer.cpp
    ready_file.cpp
    session_state.cpp
    view_manager.cpp
    views/menu_view.cpp
    views/main_menu_view.cpp
    views/manual_game_view.cpp
    views/ai_control_center_view.cpp
    views/train_cloud_ai_view.cpp
    views/hall_of_fame_view.cpp
    views/settings_view.cpp
    views/replay_vault_menu.cpp
    views/save_and_exit_view.cpp
)

add_library(snake_core ${SNAKE_CORE_SOURCES})
target_link_libraries(snake_core PUBLIC SDL3::SDL3 snake_imgui)

if(ANDROID)
    add_library(main SHARED main.cpp app_events.cpp)
    target_link_libraries(main PRIVATE snake_core log)
else()
    add_executable(snake WIN32 main.cpp)
    target_link_libraries(snake PRIVATE snake_core)
endif()
