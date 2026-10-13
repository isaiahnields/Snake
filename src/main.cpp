#include "app_events.h"
#include "game_types.h"
#include "view_manager.h"

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

    {
        ViewManager views(renderer);
        bool ready_file_created = false;

        while (!views.quit_requested())
        {
            SDL_Event event;
            while (SDL_PollEvent(&event))
            {
                views.handle_event(event);
            }

            views.update();
            views.render();

            if (!ready_file_created && !ready_file_path.empty())
            {
                std::ofstream ready_file(ready_file_path);
                if (!ready_file)
                {
                    std::cerr << "Unable to create readiness file: " << ready_file_path
                              << std::endl;
                    views.request_quit();
                }
                else
                {
                    ready_file << "ready\n";
                    ready_file_created = true;
                }
            }

            const int delay = views.frame_delay_ms();
            if (delay > 0) SDL_Delay(delay);
        }
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
