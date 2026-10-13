#include "app_events.h"
#include "game_types.h"
#include "ready_file.h"
#include "view_manager.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <iostream>

int main(int argc, char *argv[])
{
    ReadyFile::instance().configure_from_arguments(argc, argv);

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

        while (!views.quit_requested())
        {
            SDL_Event event;
            while (SDL_PollEvent(&event))
            {
                views.handle_event(event);
            }

            views.update();
            views.render();

            if (!ReadyFile::instance().upsert())
            {
                views.request_quit();
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
