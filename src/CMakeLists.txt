set(SNAKE_CORE_SOURCES
    cognito_client.cpp
    cognito_http.cpp
    gamepad_input.cpp
    snake_game.cpp
    renderer.cpp
    ready_file.cpp
    player_token_store.cpp
    session_state.cpp
    view_manager.cpp
    views/menu_view.cpp
    views/main_menu_view.cpp
    views/user_game_view.cpp
    views/ai_control_center_view.cpp
    views/ai_test_games_view.cpp
    views/ai_train_games_view.cpp
    views/ai_test_cloud_games_view.cpp
    views/ai_train_cloud_games_view.cpp
    views/hall_of_fame_view.cpp
    views/settings_view.cpp
    views/replay_vault_menu.cpp
    views/save_and_exit_view.cpp
    views/auth_form_view.cpp
    views/sign_up_or_sign_in_view.cpp
    views/sign_up_view.cpp
    views/sign_in_view.cpp
    views/password_reset_view.cpp
    views/forgot_password_view.cpp
    views/verify_account_view.cpp
    views/reset_password_view.cpp
)

add_library(snake_core ${SNAKE_CORE_SOURCES})
target_link_libraries(snake_core PUBLIC SDL3::SDL3 snake_imgui)
target_compile_features(snake_core PUBLIC cxx_std_17)
target_link_libraries(snake_core PRIVATE nlohmann_json::nlohmann_json Threads::Threads)
if(WIN32)
    target_link_libraries(snake_core PRIVATE winhttp)
elseif(NOT ANDROID)
    target_link_libraries(snake_core PRIVATE CURL::libcurl)
endif()

if(ANDROID)
    add_library(main SHARED main.cpp app_events.cpp)
    target_link_libraries(main PRIVATE snake_core log)
else()
    add_executable(snake WIN32 main.cpp)
    target_link_libraries(snake PRIVATE snake_core)
endif()
