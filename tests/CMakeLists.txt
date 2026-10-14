add_executable(test_snake_game test_snake_game.cpp)
target_link_libraries(test_snake_game PRIVATE snake_core)
add_test(NAME snake_game COMMAND test_snake_game)

add_executable(test_renderer test_renderer.cpp)
target_link_libraries(test_renderer PRIVATE snake_core)
add_test(NAME renderer COMMAND test_renderer)

add_executable(test_view_manager test_view_manager.cpp)
target_link_libraries(test_view_manager PRIVATE snake_core)
add_test(NAME view_manager COMMAND test_view_manager)

add_executable(test_gamepad_input test_gamepad_input.cpp)
target_link_libraries(test_gamepad_input PRIVATE snake_core)
add_test(NAME gamepad_input COMMAND test_gamepad_input)

add_executable(test_ready_file test_ready_file.cpp)
target_link_libraries(test_ready_file PRIVATE snake_core)
add_test(NAME ready_file COMMAND test_ready_file)

add_executable(test_auth test_auth.cpp)
target_link_libraries(test_auth PRIVATE snake_core)
add_test(NAME auth COMMAND test_auth)

foreach(target IN ITEMS test_snake_game test_renderer test_view_manager
                        test_gamepad_input test_ready_file test_auth)
    snake_copy_mingw_runtime(${target})
endforeach()

add_custom_target(all_tests DEPENDS test_snake_game test_renderer test_view_manager test_gamepad_input test_ready_file test_auth)
