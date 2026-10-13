add_executable(test_snake_game test_snake_game.cpp)
target_link_libraries(test_snake_game PRIVATE snake_core)
add_test(NAME snake_game COMMAND test_snake_game)

add_executable(test_neural_network test_neural_network.cpp)
target_link_libraries(test_neural_network PRIVATE snake_core)
add_test(NAME neural_network COMMAND test_neural_network)

add_executable(test_dqn_agent test_dqn_agent.cpp)
target_link_libraries(test_dqn_agent PRIVATE snake_core)
add_test(NAME dqn_agent COMMAND test_dqn_agent)

add_executable(test_renderer test_renderer.cpp)
target_link_libraries(test_renderer PRIVATE snake_core)
add_test(NAME renderer COMMAND test_renderer)

add_executable(test_menu test_menu.cpp)
target_link_libraries(test_menu PRIVATE snake_core)
add_test(NAME menu COMMAND test_menu)

add_custom_target(all_tests DEPENDS
    test_snake_game test_neural_network test_dqn_agent test_renderer test_menu)
