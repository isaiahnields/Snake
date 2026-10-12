#include "dqn_agent.h"
#include "renderer.h"
#include "snake_game.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

int main(int argc, char* argv[]) {
    srand(static_cast<unsigned>(time(nullptr)));

    std::string ready_file_path;
    constexpr char ready_file_argument[] = "--ready-file=";
    for (int index = 1; index < argc; ++index) {
        const std::string argument = argv[index];
        if (argument.rfind(ready_file_argument, 0) == 0) {
            ready_file_path = argument.substr(sizeof(ready_file_argument) - 1);
        }
    }

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    if (!SDL_CreateWindowAndRenderer("Snake",
                                     WINDOW_WIDTH,
                                     WINDOW_HEIGHT,
                                     0,
                                     &window,
                                     &renderer)) {
        std::cerr << "Window Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    GameMode mode = MENU;
    constexpr GameMode menu_modes[] = {MANUAL, TRAINING, AI_PLAY};
    int menu_selection = 0;
    bool quit = false;

    SnakeGame game;
    DQNAgent agent;

    int episode = 1;
    const int MANUAL_DELAY = 100;
    const int TRAINING_DELAY = 0;
    const int AI_DELAY = 100;
    bool ready_file_created = false;

    uint32_t last_move_time = SDL_GetTicks();

    while (!quit) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                quit = true;
            } else if (event.type == SDL_EVENT_KEY_DOWN) {
                if (event.key.key == SDLK_ESCAPE) {
                    if (mode != MENU) {
                        mode = MENU;
                        menu_selection = 0;
                    } else {
                        quit = true;
                    }
                } else if (mode == MENU) {
                    if (event.key.key == SDLK_UP) {
                        menu_selection = (menu_selection - 1 + 3) % 3;
                    } else if (event.key.key == SDLK_DOWN) {
                        menu_selection = (menu_selection + 1) % 3;
                    } else if (event.key.key == SDLK_RETURN || event.key.key == SDLK_SPACE) {
                        mode = menu_modes[menu_selection];
                        game.reset();
                        agent.epsilon = 1.0f;
                        episode = 1;
                        last_move_time = SDL_GetTicks();
                    }
                } else if (mode == MANUAL) {
                    if (event.key.key == SDLK_UP && game.dir != DOWN) game.dir = UP;
                    else if (event.key.key == SDLK_DOWN && game.dir != UP) game.dir = DOWN;
                    else if (event.key.key == SDLK_LEFT && game.dir != RIGHT) game.dir = LEFT;
                    else if (event.key.key == SDLK_RIGHT && game.dir != LEFT) game.dir = RIGHT;
                }
            }
        }

        if (mode == MENU) {
            draw_menu(renderer, menu_modes[menu_selection]);
            if (!ready_file_created && !ready_file_path.empty()) {
                std::ofstream ready_file(ready_file_path);
                if (!ready_file) {
                    std::cerr << "Unable to create readiness file: " << ready_file_path << std::endl;
                    quit = true;
                } else {
                    ready_file << "ready\n";
                    ready_file_created = true;
                }
            }
        } else if (mode == MANUAL) {
            uint32_t current_time = SDL_GetTicks();
            if (current_time - last_move_time >= MANUAL_DELAY) {
                game.step(STRAIGHT);

                if (game.game_over) {
                    std::cout << "Game Over! Score: " << game.score << std::endl;
                    game.reset();
                }

                last_move_time = current_time;
            }

            draw_game(renderer, game);
            SDL_Delay(16);
        } else if (mode == TRAINING) {
            std::vector<float> state = game.get_state_vector();
            Action action = agent.get_action(state, true);
            float reward = game.step(action);
            std::vector<float> next_state = game.get_state_vector();

            agent.store_transition(state, action, reward, next_state, game.game_over);
            agent.train();

            if (game.game_over) {
                agent.decay_epsilon();
                if (episode % 50 == 0) {
                    std::cout << "Episode: " << episode
                              << " | Score: " << game.score
                              << " | Epsilon: " << agent.epsilon
                              << std::endl;
                }
                game.reset();
                ++episode;
            }

            if (episode % 10 == 0) {
                draw_game(renderer, game);
            }
            SDL_Delay(TRAINING_DELAY);
        } else if (mode == AI_PLAY) {
            uint32_t current_time = SDL_GetTicks();
            if (current_time - last_move_time >= AI_DELAY) {
                std::vector<float> state = game.get_state_vector();
                Action action = agent.get_action(state, false);
                game.step(action);

                if (game.game_over) {
                    std::cout << "AI Game Over! Score: " << game.score << std::endl;
                    game.reset();
                    episode++;
                }

                last_move_time = current_time;
            }

            draw_game(renderer, game);
            SDL_Delay(16);
        }
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}