#include "test_helpers.h"

#include "../src/renderer.h"
#include "../src/view_manager.h"
#include "../src/views/manual_game_view.h"

#include <SDL3/SDL.h>

static bool pixel_is(SDL_Surface *s, int x, int y, Uint8 r, Uint8 g, Uint8 b)
{
    Uint8 pr, pg, pb, pa;
    if (!SDL_ReadSurfacePixel(s, x, y, &pr, &pg, &pb, &pa)) return false;
    return pr == r && pg == g && pb == b;
}

static void press(ViewManager &manager, SDL_Keycode key)
{
    SDL_Event event{};
    event.type = SDL_EVENT_KEY_DOWN;
    event.key.key = key;
    manager.handle_event(event);
}

static bool render_scale_is_reset(SDL_Renderer *renderer)
{
    float scale_x = 0.0f, scale_y = 0.0f;
    return SDL_GetRenderScale(renderer, &scale_x, &scale_y) && scale_x == 1.0f && scale_y == 1.0f;
}

int main()
{
    SDL_Surface *surface = SDL_CreateSurface(WINDOW_WIDTH, WINDOW_HEIGHT, SDL_PIXELFORMAT_RGBA32);
    CHECK(surface != nullptr);
    if (!surface) return 1;
    SDL_Renderer *renderer = SDL_CreateSoftwareRenderer(surface);
    CHECK(renderer != nullptr);
    if (!renderer) return 1;

    ViewManager manager(renderer);
    MenuView &menu = *manager.active().as_menu();
    menu.selection = 2;
    manager.render();
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
        manager.open_page(static_cast<MenuPage>(page));
        const MenuView *active = manager.active().as_menu();
        CHECK(active != nullptr);
        if (active == nullptr) continue;
        manager.active().render(renderer);
        CHECK(render_scale_is_reset(renderer));
        manager.render();
        CHECK(pixel_is(surface, 42, 42, 100, 200, 255));
        CHECK(MENU_TOP + active->items().size() * MENU_ROW_HEIGHT < 770);
        for (const auto &item : active->items())
            CHECK((item.label.size() + 3) * SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * 1.25f <
                  MENU_ROW_WIDTH - 24);
        CHECK(active->details().size() <= 5);
        for (const auto &line : active->details())
            CHECK(MENU_LEFT + line.size() * SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * 1.25f <
                  WINDOW_WIDTH);
    }

    // Game: food, head and body colors.
    manager.open_page(MenuPage::Play);
    press(manager, SDLK_RETURN);
    auto &manual = static_cast<ManualGameView &>(manager.view(ViewId::ManualGame));
    CHECK(manual.playing());
    SnakeGame &game = manual.game();
    game.snake = {{2, 2}, {1, 2}};
    game.food = {10, 10};
    manager.render();
    CHECK(pixel_is(surface, 1, 1, 30, 30, 30));
    CHECK(pixel_is(surface, 10 * CELL_SIZE + 5, 10 * CELL_SIZE + 5, 255, 50, 50));
    CHECK(pixel_is(surface, 2 * CELL_SIZE + 5, 2 * CELL_SIZE + 5, 50, 255, 50));
    CHECK(pixel_is(surface, 1 * CELL_SIZE + 5, 2 * CELL_SIZE + 5, 30, 150, 30));

    // AI views: visual training and testing draw the board; headless shows a progress screen.
    manager.open(ViewId::TrainAI);
    manager.render();
    CHECK(render_scale_is_reset(renderer));
    CHECK(pixel_is(surface, (GRID_WIDTH / 2) * CELL_SIZE + 5, (GRID_HEIGHT / 2) * CELL_SIZE + 5, 50,
                   255, 50));
    manager.session().visual_training = false;
    manager.active().render(renderer);
    CHECK(render_scale_is_reset(renderer));
    manager.render();
    CHECK(pixel_is(surface, 42, 42, 100, 200, 255));
    manager.open(ViewId::TestAI);
    manager.render();
    CHECK(pixel_is(surface, (GRID_WIDTH / 2) * CELL_SIZE + 5, (GRID_HEIGHT / 2) * CELL_SIZE + 5, 50,
                   255, 50));

    SDL_DestroyRenderer(renderer);
    SDL_DestroySurface(surface);
    return TEST_RESULT();
}
