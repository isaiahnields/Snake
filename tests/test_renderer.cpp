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

    Menu menu;
    menu.selection = 2;
    draw_menu(renderer, menu);
    CHECK(pixel_is(surface, 1, 1, 30, 30, 30));
    CHECK(pixel_is(surface, 42, 42, 100, 200, 255));
    CHECK(pixel_is(surface, 42, 292, 45, 45, 45));
    CHECK(pixel_is(surface, 42, 396, 50, 120, 85));
    CHECK(menu_item_at(MENU_LEFT + 2, MENU_TOP + 2, menu) == 0);
    CHECK(menu_item_at(MENU_LEFT + 2, MENU_TOP + 2 * MENU_ROW_HEIGHT + 2, menu) == 2);
    CHECK(menu_item_at(MENU_LEFT - 1, MENU_TOP, menu) == -1);
    CHECK(menu_item_at(MENU_LEFT + MENU_ROW_WIDTH, MENU_TOP, menu) == -1);
    CHECK(menu_item_at(MENU_LEFT, MENU_TOP - 1, menu) == -1);
    CHECK(menu_item_at(MENU_LEFT, MENU_TOP + MENU_ROW_HEIGHT - 1, menu) == -1);
    CHECK(menu_item_at(MENU_LEFT, MENU_TOP + 6 * MENU_ROW_HEIGHT, menu) == -1);

    for (int page = static_cast<int>(MenuPage::Main); page <= static_cast<int>(MenuPage::Exit);
         ++page)
    {
        menu.open(static_cast<MenuPage>(page));
        draw_menu(renderer, menu);
        float scale_x, scale_y;
        CHECK(SDL_GetRenderScale(renderer, &scale_x, &scale_y));
        CHECK(scale_x == 1.0f && scale_y == 1.0f);
        CHECK(MENU_TOP + menu.items().size() * MENU_ROW_HEIGHT < 770);
        for (const auto &item : menu.items())
            CHECK((item.label.size() + 3) * SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * 1.25f <
                  MENU_ROW_WIDTH - 24);
    }

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
