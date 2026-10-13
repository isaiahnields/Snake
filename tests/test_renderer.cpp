#include "test_helpers.h"

#include "../src/renderer.h"

#include <SDL3/SDL.h>

static bool pixel_is(SDL_Surface *s, int x, int y, Uint8 r, Uint8 g, Uint8 b)
{
    Uint8 pr, pg, pb, pa;
    if (!SDL_ReadSurfacePixel(s, x, y, &pr, &pg, &pb, &pa)) return false;
    return pr == r && pg == g && pb == b;
}

int main()
{
    SDL_Surface *surface = SDL_CreateSurface(WINDOW_WIDTH, WINDOW_HEIGHT, SDL_PIXELFORMAT_RGBA32);
    CHECK(surface != nullptr);
    if (!surface) return 1;
    SDL_Renderer *renderer = SDL_CreateSoftwareRenderer(surface);
    CHECK(renderer != nullptr);
    if (!renderer) return 1;

    // Menu: background plus highlighted option for the selected mode.
    draw_menu(renderer, TRAINING);
    CHECK(pixel_is(surface, 1, 1, 30, 30, 30));
    const int menu_x = (WINDOW_WIDTH - 300) / 2;
    const int menu_y = (WINDOW_HEIGHT - 350) / 2;
    CHECK(pixel_is(surface, menu_x + 2, menu_y + 2, 100, 200, 255));
    const int options_y = menu_y + 60 + 40;
    CHECK(pixel_is(surface, menu_x + 2, options_y + 2, 100, 150, 100));
    CHECK(pixel_is(surface, menu_x + 2, options_y + 100 + 2, 100, 255, 100));

    // Game: food, head and body colors.
    SnakeGame game;
    game.snake.clear();
    game.snake = {{2, 2}, {1, 2}};
    game.food = {10, 10};
    draw_game(renderer, game);
    CHECK(pixel_is(surface, 1, 1, 30, 30, 30));
    CHECK(pixel_is(surface, 10 * CELL_SIZE + 5, 10 * CELL_SIZE + 5, 255, 50, 50));
    CHECK(pixel_is(surface, 2 * CELL_SIZE + 5, 2 * CELL_SIZE + 5, 50, 255, 50));
    CHECK(pixel_is(surface, 1 * CELL_SIZE + 5, 2 * CELL_SIZE + 5, 30, 150, 30));

    SDL_DestroyRenderer(renderer);
    SDL_DestroySurface(surface);
    return TEST_RESULT();
}
