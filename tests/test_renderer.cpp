#include "test_helpers.h"

#include "../src/renderer.h"
#include "../src/view_manager.h"
#include "../src/views/manual_game_view.h"

#include <SDL3/SDL.h>
#include <imgui.h>

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

static void mouse_event(ViewManager &manager, Uint32 type, float x, float y,
                        Uint8 button = SDL_BUTTON_LEFT, SDL_WindowID window_id = 0)
{
    SDL_Event event{};
    event.type = type;
    if (type == SDL_EVENT_MOUSE_MOTION)
    {
        event.motion.windowID = window_id;
        event.motion.x = x;
        event.motion.y = y;
    }
    else
    {
        event.button.windowID = window_id;
        event.button.button = button;
        event.button.x = x;
        event.button.y = y;
    }
    manager.handle_event(event);
    manager.render();
}

static void click(ViewManager &manager, float x, float y, Uint8 button = SDL_BUTTON_LEFT,
                  SDL_WindowID window_id = 0)
{
    mouse_event(manager, SDL_EVENT_MOUSE_MOTION, x, y, button, window_id);
    mouse_event(manager, SDL_EVENT_MOUSE_BUTTON_DOWN, x, y, button, window_id);
    mouse_event(manager, SDL_EVENT_MOUSE_BUTTON_UP, x, y, button, window_id);
}

static void touch(ViewManager &manager, Uint32 type, SDL_FingerID finger, float x, float y,
                  int width = WINDOW_WIDTH, int height = WINDOW_HEIGHT, SDL_WindowID window_id = 0)
{
    SDL_Event event{};
    event.type = type;
    event.tfinger.touchID = 1;
    event.tfinger.fingerID = finger;
    event.tfinger.windowID = window_id;
    event.tfinger.x = x / width;
    event.tfinger.y = y / height;
    manager.handle_event(event);
    manager.render();
}

static void test_ui_and_board(SDL_Renderer *renderer, SDL_Surface *surface)
{
    ViewManager manager(renderer);
    CHECK(manager.initialize_ui());
    CHECK(ImGui::GetIO().IniFilename == nullptr);
    MenuView &menu = *manager.active().as_menu();
    menu.selection = 2;
    manager.render();
    CHECK(ImGui::GetDrawData()->TotalVtxCount > 0);
    CHECK(pixel_is(surface, 42, 292, 45, 45, 45));
    CHECK(pixel_is(surface, 60, 405, 50, 120, 85));

    for (int page = static_cast<int>(MenuPage::Main); page <= static_cast<int>(MenuPage::Exit);
         ++page)
    {
        manager.open_page(static_cast<MenuPage>(page));
        const MenuView *active = manager.active().as_menu();
        CHECK(active != nullptr);
        if (active == nullptr) continue;
        manager.render();
        CHECK(render_scale_is_reset(renderer));
        CHECK(ImGui::GetDrawData()->TotalVtxCount > 0);
        CHECK(ImGui::GetDrawData()->DisplaySize.x == WINDOW_WIDTH);
        CHECK(ImGui::GetDrawData()->DisplaySize.y == WINDOW_HEIGHT);
        CHECK(MENU_TOP + active->items().size() * MENU_ROW_HEIGHT < 770);
    }

    // ImGui owns hit testing and activates only on release inside the pressed button.
    manager.open_page(MenuPage::Main);
    manager.render();
    click(manager, MENU_LEFT - 1, MENU_TOP + 20);
    CHECK(manager.active_id() == ViewId::MainMenu);
    click(manager, MENU_LEFT + 20, MENU_TOP + 20, SDL_BUTTON_RIGHT);
    CHECK(manager.active_id() == ViewId::MainMenu);
    mouse_event(manager, SDL_EVENT_MOUSE_MOTION, MENU_LEFT + 20, MENU_TOP + 20);
    mouse_event(manager, SDL_EVENT_MOUSE_BUTTON_DOWN, MENU_LEFT + 20, MENU_TOP + 20);
    CHECK(manager.active_id() == ViewId::MainMenu);
    mouse_event(manager, SDL_EVENT_MOUSE_MOTION, MENU_LEFT - 1, MENU_TOP + 20);
    mouse_event(manager, SDL_EVENT_MOUSE_BUTTON_UP, MENU_LEFT - 1, MENU_TOP + 20);
    CHECK(manager.active_id() == ViewId::MainMenu);
    click(manager, MENU_LEFT + 20, MENU_TOP + MENU_ROW_HEIGHT + 20);
    CHECK(manager.active_id() == ViewId::HallOfFame);

    // Unavailable actions remain clickable so they still explain that nothing changed.
    manager.open_page(MenuPage::Difficulty);
    manager.render();
    click(manager, MENU_LEFT + 20, MENU_TOP + 20);
    CHECK(manager.active().as_menu()->page == MenuPage::Difficulty);
    CHECK(!manager.active().as_menu()->notice.empty());

    manager.open_page(MenuPage::Main);
    manager.render();
    touch(manager, SDL_EVENT_FINGER_DOWN, 3, MENU_LEFT + 20, MENU_TOP + 20);
    touch(manager, SDL_EVENT_FINGER_CANCELED, 3, MENU_LEFT + 20, MENU_TOP + 20);
    manager.render();
    CHECK(manager.active_id() == ViewId::MainMenu);
    touch(manager, SDL_EVENT_FINGER_DOWN, 1, MENU_LEFT + 20, MENU_TOP + 20);
    touch(manager, SDL_EVENT_FINGER_DOWN, 2, MENU_LEFT + 20, MENU_TOP + MENU_ROW_HEIGHT + 20);
    touch(manager, SDL_EVENT_FINGER_UP, 2, MENU_LEFT + 20, MENU_TOP + MENU_ROW_HEIGHT + 20);
    CHECK(manager.active_id() == ViewId::MainMenu);
    touch(manager, SDL_EVENT_FINGER_UP, 1, MENU_LEFT + 20, MENU_TOP + 20);
    manager.render();
    CHECK(manager.active_id() == ViewId::ManualGame);
    CHECK(!static_cast<ManualGameView &>(manager.active()).playing());

    // SDL's synthetic touch-to-mouse event must not activate a second screen.
    SDL_Event synthetic{};
    synthetic.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
    synthetic.button.which = SDL_TOUCH_MOUSEID;
    synthetic.button.button = SDL_BUTTON_LEFT;
    synthetic.button.x = MENU_LEFT + 20;
    synthetic.button.y = MENU_TOP + 20;
    manager.handle_event(synthetic);
    synthetic.type = SDL_EVENT_MOUSE_BUTTON_UP;
    manager.handle_event(synthetic);
    manager.render();
    CHECK(!static_cast<ManualGameView &>(manager.active()).playing());

    // Game rendering and keyboard controls retain their existing behavior.
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

    manager.open(ViewId::TrainAI);
    manager.render();
    CHECK(render_scale_is_reset(renderer));
    CHECK(ImGui::GetDrawData()->TotalVtxCount > 0);
    CHECK(pixel_is(surface, (GRID_WIDTH / 2) * CELL_SIZE + 5, (GRID_HEIGHT / 2) * CELL_SIZE + 5, 50,
                   255, 50));
    manager.session().visual_training = false;
    manager.render();
    CHECK(render_scale_is_reset(renderer));
    CHECK(ImGui::GetDrawData()->TotalVtxCount > 0);
    manager.open(ViewId::TestAI);
    manager.render();
    CHECK(ImGui::GetDrawData()->TotalVtxCount > 0);
    CHECK(pixel_is(surface, (GRID_WIDTH / 2) * CELL_SIZE + 5, (GRID_HEIGHT / 2) * CELL_SIZE + 5, 50,
                   255, 50));
}

static void test_window_backend()
{
    CHECK(SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy"));
    CHECK(SDL_Init(SDL_INIT_VIDEO));
    SDL_Window *window = nullptr;
    SDL_Renderer *renderer = nullptr;
    CHECK(SDL_CreateWindowAndRenderer("ImGui test", WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_HIDDEN,
                                      &window, &renderer));
    if (renderer != nullptr)
    {
        {
            ViewManager manager(renderer);
            CHECK(manager.initialize_ui());
            CHECK(SDL_SetRenderLogicalPresentation(renderer, WINDOW_WIDTH, WINDOW_HEIGHT,
                                                   SDL_LOGICAL_PRESENTATION_LETTERBOX));
            manager.render();
            CHECK(ImGui::GetIO().BackendPlatformUserData != nullptr);
            CHECK(ImGui::GetIO().BackendRendererUserData != nullptr);
            CHECK(SDL_SetWindowSize(window, 1200, 600));
            manager.render();
            CHECK(ImGui::GetDrawData()->DisplaySize.x == 1200);
            CHECK(ImGui::GetDrawData()->DisplaySize.y == 600);
            const float scale = 600.0f / WINDOW_HEIGHT;
            click(manager, (MENU_LEFT + 20) * scale, (MENU_TOP + MENU_ROW_HEIGHT + 20) * scale,
                  SDL_BUTTON_LEFT, SDL_GetWindowID(window));
            CHECK(manager.active_id() == ViewId::HallOfFame);
            manager.open_page(MenuPage::Main);
            manager.render();
            touch(manager, SDL_EVENT_FINGER_DOWN, 1, (MENU_LEFT + 20) * scale,
                  (MENU_TOP + 20) * scale, 1200, 600, SDL_GetWindowID(window));
            touch(manager, SDL_EVENT_FINGER_UP, 1, (MENU_LEFT + 20) * scale,
                  (MENU_TOP + 20) * scale, 1200, 600, SDL_GetWindowID(window));
            manager.render();
            CHECK(manager.active_id() == ViewId::ManualGame);
            CHECK(!static_cast<ManualGameView &>(manager.active()).playing());
            int width = 0, height = 0;
            SDL_RendererLogicalPresentation presentation;
            CHECK(SDL_GetRenderLogicalPresentation(renderer, &width, &height, &presentation));
            CHECK(width == WINDOW_WIDTH && height == WINDOW_HEIGHT);
            CHECK(presentation == SDL_LOGICAL_PRESENTATION_LETTERBOX);
            CHECK(render_scale_is_reset(renderer));
        }
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
    }
    SDL_Quit();
}

int main()
{
    SDL_Surface *surface = SDL_CreateSurface(WINDOW_WIDTH, WINDOW_HEIGHT, SDL_PIXELFORMAT_RGBA32);
    CHECK(surface != nullptr);
    if (!surface) return 1;
    SDL_Renderer *renderer = SDL_CreateSoftwareRenderer(surface);
    CHECK(renderer != nullptr);
    if (!renderer)
    {
        SDL_DestroySurface(surface);
        return 1;
    }
    test_ui_and_board(renderer, surface);
    SDL_DestroyRenderer(renderer);
    SDL_DestroySurface(surface);
    test_window_backend();
    return TEST_RESULT();
}
