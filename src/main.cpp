#include "app_events.h"
#include "renderer.h"
#include "snake_game.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <fstream>
#include <iostream>
#include <string>

int main(int argc, char *argv[])
{
    std::string ready_file_path;
    constexpr char ready_file_argument[] = "--ready-file=";
    for (int index = 1; index < argc; ++index)
    {
        const std::string argument = argv[index];
        if (argument.rfind(ready_file_argument, 0) == 0)
        {
            ready_file_path = argument.substr(sizeof(ready_file_argument) - 1);
        }
    }

#ifdef SDL_PLATFORM_ANDROID
    SDL_SetEventFilter(HandleAppEvents, nullptr);
#endif

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_Window *window = nullptr;
    SDL_Renderer *renderer = nullptr;
    if (!SDL_CreateWindowAndRenderer("Snake", WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_RESIZABLE,
                                     &window, &renderer))
    {
        std::cerr << "Window Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }
    if (!SDL_SetRenderLogicalPresentation(renderer, WINDOW_WIDTH, WINDOW_HEIGHT,
                                          SDL_LOGICAL_PRESENTATION_LETTERBOX))
    {
        std::cerr << "Render presentation Error: " << SDL_GetError() << std::endl;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    GameMode mode = MENU;
    Menu menu;
    bool quit = false;

    SnakeGame game;

    const int MANUAL_DELAY = 100;
    bool ready_file_created = false;

    uint32_t last_move_time = SDL_GetTicks();
    const auto activate_menu = [&](MenuAction action)
    {
        if (action == MenuAction::Quit)
        {
            quit = true;
            return;
        }
        if (action != MenuAction::Manual) return;
        mode = MANUAL;
        game.reset();
        last_move_time = SDL_GetTicks();
    };

    while (!quit)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_QUIT)
            {
                mode = MENU;
                menu.open(MenuPage::Exit);
            }
            else if (mode == MENU && (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN ||
                                      event.type == SDL_EVENT_FINGER_DOWN))
            {
                if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
                    (event.button.button != SDL_BUTTON_LEFT ||
                     event.button.which == SDL_TOUCH_MOUSEID))
                    continue;
                if (event.type == SDL_EVENT_FINGER_DOWN &&
                    event.tfinger.touchID == SDL_MOUSE_TOUCHID)
                    continue;
                if (!SDL_ConvertEventToRenderCoordinates(renderer, &event))
                {
                    std::cerr << "Menu input conversion error: " << SDL_GetError() << std::endl;
                    continue;
                }
                const float x =
                    event.type == SDL_EVENT_MOUSE_BUTTON_DOWN ? event.button.x : event.tfinger.x;
                const float y =
                    event.type == SDL_EVENT_MOUSE_BUTTON_DOWN ? event.button.y : event.tfinger.y;
                const int selected = menu_item_at(x, y, menu);
                if (selected >= 0)
                {
                    menu.selection = selected;
                    activate_menu(menu.activate());
                }
            }
            else if (event.type == SDL_EVENT_KEY_DOWN)
            {
                if (event.key.repeat) continue;
                if (event.key.key == SDLK_ESCAPE)
                {
                    if (mode != MENU)
                    {
                        menu.notice = "Manual game stopped.";
                        mode = MENU;
                    }
                    else
                    {
                        menu.back();
                    }
                }
                else if (mode == MENU)
                {
                    if (event.key.key == SDLK_UP)
                    {
                        menu.move(-1);
                    }
                    else if (event.key.key == SDLK_DOWN)
                    {
                        menu.move(1);
                    }
                    else if (event.key.key == SDLK_RETURN || event.key.key == SDLK_SPACE)
                    {
                        activate_menu(menu.activate());
                    }
                    else if (event.key.key >= SDLK_0 && event.key.key <= SDLK_9)
                    {
                        activate_menu(menu.shortcut(static_cast<int>(event.key.key - SDLK_0)));
                    }
                }
                else if (mode == MANUAL)
                {
                    if (event.key.key == SDLK_UP && game.dir != DOWN)
                        game.dir = UP;
                    else if (event.key.key == SDLK_DOWN && game.dir != UP)
                        game.dir = DOWN;
                    else if (event.key.key == SDLK_LEFT && game.dir != RIGHT)
                        game.dir = LEFT;
                    else if (event.key.key == SDLK_RIGHT && game.dir != LEFT)
                        game.dir = RIGHT;
                }
            }
        }

        if (mode == MENU)
        {
            draw_menu(renderer, menu);
            if (!ready_file_created && !ready_file_path.empty())
            {
                std::ofstream ready_file(ready_file_path);
                if (!ready_file)
                {
                    std::cerr << "Unable to create readiness file: " << ready_file_path
                              << std::endl;
                    quit = true;
                }
                else
                {
                    ready_file << "ready\n";
                    ready_file_created = true;
                }
            }
            SDL_Delay(16);
        }
        else if (mode == MANUAL)
        {
            uint32_t current_time = SDL_GetTicks();
            if (current_time - last_move_time >= MANUAL_DELAY)
            {
                game.step(STRAIGHT);

                if (game.game_over)
                {
                    std::cout << "Game Over! Score: " << game.score << std::endl;
                    game.reset();
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
