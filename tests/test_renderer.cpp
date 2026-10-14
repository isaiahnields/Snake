#include "test_helpers.h"

#include "../src/renderer.h"
#include "../src/view_manager.h"
#include "../src/views/user_game_view.h"

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

static void check_menu_has_no_selection(SDL_Surface *surface, const ViewManager &manager)
{
    for (int y = 0; y < surface->h; ++y)
    {
        if (!pixel_is(surface, static_cast<int>(MENU_LEFT) + 2, y, 50, 120, 85)) continue;
        std::fprintf(stderr, "Unexpected selection in view %d at y=%d\n",
                     static_cast<int>(manager.active_id()), y);
        CHECK(false);
        return;
    }
}

static void test_no_navigation_devices(SDL_Renderer *renderer, SDL_Surface *surface)
{
    CHECK(!SDL_HasKeyboard());
    CHECK(!SDL_HasGamepad());
    ViewManager manager(renderer);
    CHECK(manager.initialize_ui());
    CHECK(manager.render());
    check_menu_has_no_selection(surface, manager);
    int sign_up_y = -1;
    for (int y = 0; y < surface->h; ++y)
    {
        if (!pixel_is(surface, static_cast<int>(MENU_LEFT) + 2, y, 45, 45, 45)) continue;
        sign_up_y = y + 20;
        break;
    }
    CHECK(sign_up_y >= 0);
    if (sign_up_y < 0) return;
    click(manager, MENU_LEFT + 20, static_cast<float>(sign_up_y));
    CHECK(manager.active_id() == ViewId::SignUp);
    manager.open(ViewId::SignUpOrSignIn);
    CHECK(manager.render());
    check_menu_has_no_selection(surface, manager);

    authenticate_test_player(manager);
    for (int page = static_cast<int>(MenuPage::Main); page <= static_cast<int>(MenuPage::Exit);
         ++page)
    {
        manager.open_page(static_cast<MenuPage>(page));
        CHECK(manager.render());
        check_menu_has_no_selection(surface, manager);
    }
    manager.open_page(MenuPage::Main);
    CHECK(manager.render());
    touch(manager, SDL_EVENT_FINGER_DOWN, 1, MENU_LEFT + 20, MENU_TOP + MENU_ROW_HEIGHT + 20);
    touch(manager, SDL_EVENT_FINGER_UP, 1, MENU_LEFT + 20, MENU_TOP + MENU_ROW_HEIGHT + 20);
    CHECK(manager.active_id() == ViewId::AIControlCenter);
    CHECK(manager.render());
    check_menu_has_no_selection(surface, manager);
}

static SDL_JoystickID attach_test_gamepad()
{
    SDL_VirtualJoystickDesc desc{};
    SDL_INIT_INTERFACE(&desc);
    desc.type = SDL_JOYSTICK_TYPE_GAMEPAD;
    desc.nbuttons = SDL_GAMEPAD_BUTTON_COUNT;
    desc.naxes = SDL_GAMEPAD_AXIS_COUNT;
    desc.name = "Menu selection test gamepad";
    const SDL_JoystickID id = SDL_AttachVirtualJoystick(&desc);
    CHECK(id != 0);
    CHECK(SDL_HasGamepad());
    return id;
}

static void test_gamepad_connection(SDL_Renderer *renderer, SDL_Surface *surface)
{
    ViewManager manager(renderer);
    authenticate_test_player(manager);
    CHECK(manager.initialize_ui());
    MenuView &menu = *manager.active().as_menu();
    menu.selection = 2;
    const bool already_connected = SDL_HasKeyboard() || SDL_HasGamepad();
    CHECK(manager.render());
    CHECK(pixel_is(surface, 60, 405, already_connected ? 50 : 45, already_connected ? 120 : 45,
                   already_connected ? 85 : 45));
    const SDL_JoystickID id = attach_test_gamepad();
    CHECK(manager.render());
    CHECK(menu.selection == 2);
    CHECK(pixel_is(surface, 60, 405, 50, 120, 85));
    CHECK(SDL_DetachVirtualJoystick(id));
    CHECK(manager.render());
    CHECK(menu.selection == 2);
    CHECK(pixel_is(surface, 60, 405, already_connected ? 50 : 45, already_connected ? 120 : 45,
                   already_connected ? 85 : 45));
}

static void test_account_selection(SDL_Renderer *renderer, SDL_Surface *surface)
{
    ViewManager manager(renderer);
    CHECK(manager.initialize_ui());
    manager.render();
    int sign_up_y = -1;
    int sign_in_y = -1;
    const int x = static_cast<int>(MENU_LEFT) + 2;
    for (int y = 0; y < surface->h; ++y)
    {
        if (sign_up_y < 0 && pixel_is(surface, x, y, 50, 120, 85)) sign_up_y = y + 20;
        if (sign_in_y < 0 && pixel_is(surface, x, y, 45, 45, 45)) sign_in_y = y + 20;
    }
    CHECK(sign_up_y >= 0 && sign_in_y > sign_up_y);
    if (sign_up_y < 0 || sign_in_y < 0) return;
    CHECK(pixel_is(surface, x, sign_up_y, 50, 120, 85));
    CHECK(pixel_is(surface, x, sign_in_y, 45, 45, 45));
    press(manager, SDLK_DOWN);
    manager.render();
    CHECK(pixel_is(surface, x, sign_up_y, 45, 45, 45));
    CHECK(pixel_is(surface, x, sign_in_y, 50, 120, 85));
    press(manager, SDLK_RETURN);
    CHECK(manager.active_id() == ViewId::SignIn);
    manager.open(ViewId::SignUpOrSignIn);
    press(manager, SDLK_UP);
    manager.render();
    CHECK(pixel_is(surface, x, sign_up_y, 50, 120, 85));
    CHECK(pixel_is(surface, x, sign_in_y, 45, 45, 45));
    press(manager, SDLK_SPACE);
    CHECK(manager.active_id() == ViewId::SignUp);
    manager.open(ViewId::SignUpOrSignIn);
    press(manager, SDLK_W);
    manager.render();
    CHECK(pixel_is(surface, x, sign_up_y, 45, 45, 45));
    CHECK(pixel_is(surface, x, sign_in_y, 50, 120, 85));
    press(manager, SDLK_RETURN);
    CHECK(manager.active_id() == ViewId::SignIn);
    manager.open(ViewId::SignUpOrSignIn);
    press(manager, SDLK_S);
    manager.render();
    CHECK(pixel_is(surface, x, sign_up_y, 50, 120, 85));
    CHECK(pixel_is(surface, x, sign_in_y, 45, 45, 45));
    press(manager, SDLK_SPACE);
    CHECK(manager.active_id() == ViewId::SignUp);
    manager.open(ViewId::SignUpOrSignIn);
    manager.render();
    click(manager, MENU_LEFT + 20, static_cast<float>(sign_in_y));
    CHECK(manager.active_id() == ViewId::SignIn);
    manager.open(ViewId::SignUpOrSignIn);
    manager.render();
    click(manager, MENU_LEFT + 20, static_cast<float>(sign_up_y));
    CHECK(manager.active_id() == ViewId::SignUp);
}

static void test_ui_and_board(SDL_Renderer *renderer, SDL_Surface *surface)
{
    ViewManager manager(renderer);
    authenticate_test_player(manager);
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
    CHECK(manager.active_id() == ViewId::UserGame);
    CHECK(!static_cast<UserGameView &>(manager.active()).playing());

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
    CHECK(!static_cast<UserGameView &>(manager.active()).playing());

    // Game rendering and keyboard controls retain their existing behavior.
    press(manager, SDLK_RETURN);
    auto &manual = static_cast<UserGameView &>(manager.view(ViewId::UserGame));
    CHECK(manual.playing());
    SnakeGame &game = manual.game();
    game.snake = {{2, 2}, {1, 2}};
    game.food = {10, 10};
    manager.render();
    CHECK(pixel_is(surface, 1, 1, 30, 30, 30));
    CHECK(pixel_is(surface, 10 * CELL_SIZE + 5, 10 * CELL_SIZE + 5, 255, 50, 50));
    CHECK(pixel_is(surface, 2 * CELL_SIZE + 5, 2 * CELL_SIZE + 5, 50, 255, 50));
    CHECK(pixel_is(surface, 1 * CELL_SIZE + 5, 2 * CELL_SIZE + 5, 30, 150, 30));

    manager.open_page(MenuPage::Train);
    manager.render();
    CHECK(render_scale_is_reset(renderer));
    CHECK(ImGui::GetDrawData()->TotalVtxCount > 0);
    click(manager, MENU_LEFT + 20, MENU_TOP + 2 * MENU_ROW_HEIGHT + 20);
    CHECK(manager.active_id() == ViewId::AITrainGames);
    CHECK(manager.active().as_menu()->page == MenuPage::Train);
    CHECK(manager.active().as_menu()->notice == "Not implemented yet. No changes were made.");
    press(manager, SDLK_2);
    CHECK(!manager.session().visual_training);
    manager.render();
    CHECK(render_scale_is_reset(renderer));
    CHECK(ImGui::GetDrawData()->TotalVtxCount > 0);
    click(manager, MENU_LEFT + 20, MENU_TOP + 2 * MENU_ROW_HEIGHT + 20);
    CHECK(manager.active_id() == ViewId::AITrainGames);
    CHECK(manager.active().as_menu()->page == MenuPage::Train);
    CHECK(manager.active().as_menu()->notice == "Not implemented yet. No changes were made.");

    manager.open_page(MenuPage::Test);
    manager.render();
    CHECK(ImGui::GetDrawData()->TotalVtxCount > 0);
    touch(manager, SDL_EVENT_FINGER_DOWN, 1, MENU_LEFT + 20, MENU_TOP + 20);
    touch(manager, SDL_EVENT_FINGER_UP, 1, MENU_LEFT + 20, MENU_TOP + 20);
    CHECK(manager.active_id() == ViewId::AITestGames);
    CHECK(manager.active().as_menu()->page == MenuPage::Test);
    CHECK(manager.active().as_menu()->notice == "Not implemented yet. No changes were made.");
    CHECK(manager.session().best_ai_scores.empty());
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
            authenticate_test_player(manager);
            CHECK(manager.initialize_ui());
            CHECK(SDL_SetRenderLogicalPresentation(renderer, WINDOW_WIDTH, WINDOW_HEIGHT,
                                                   SDL_LOGICAL_PRESENTATION_LETTERBOX));
            manager.render();
            CHECK(ImGui::GetIO().BackendPlatformUserData != nullptr);
            CHECK(ImGui::GetIO().BackendRendererUserData != nullptr);
            CHECK(SDL_SetWindowSize(window, 1200, 600));
            SDL_PumpEvents();
            manager.render();
            CHECK(ImGui::GetDrawData()->DisplaySize.x == 1200);
            CHECK(ImGui::GetDrawData()->DisplaySize.y == 600);
            const float scale = 600.0f / WINDOW_HEIGHT;
            const float offset_x = (1200.0f - WINDOW_WIDTH * scale) / 2.0f;
            click(manager, offset_x + (MENU_LEFT + 20) * scale,
                  (MENU_TOP + MENU_ROW_HEIGHT + 20) * scale, SDL_BUTTON_LEFT,
                  SDL_GetWindowID(window));
            CHECK(manager.active_id() == ViewId::AIControlCenter);
            manager.open_page(MenuPage::Main);
            manager.render();
            touch(manager, SDL_EVENT_FINGER_DOWN, 1, offset_x + (MENU_LEFT + 20) * scale,
                  (MENU_TOP + 20) * scale, 1200, 600, SDL_GetWindowID(window));
            touch(manager, SDL_EVENT_FINGER_UP, 1, offset_x + (MENU_LEFT + 20) * scale,
                  (MENU_TOP + 20) * scale, 1200, 600, SDL_GetWindowID(window));
            manager.render();
            CHECK(manager.active_id() == ViewId::UserGame);
            CHECK(!static_cast<UserGameView &>(manager.active()).playing());
            press(manager, SDLK_1);
            auto &manual = static_cast<UserGameView &>(manager.active());
            const SDL_WindowID window_id = SDL_GetWindowID(window);
            for (const auto &size : {Point{1200, 600}, Point{600, 1200}})
            {
                CHECK(SDL_SetWindowSize(window, size.x, size.y));
                SDL_PumpEvents();
                manual.game().dir = RIGHT;
                touch(manager, SDL_EVENT_FINGER_DOWN, 1, 300, 300, size.x, size.y, window_id);
                touch(manager, SDL_EVENT_FINGER_MOTION, 1, 300, 280, size.x, size.y, window_id);
                CHECK(manual.game().dir == RIGHT);
                touch(manager, SDL_EVENT_FINGER_MOTION, 1, 330, 250, size.x, size.y, window_id);
                CHECK(manual.game().dir == UP);
                touch(manager, SDL_EVENT_FINGER_UP, 1, 330, 250, size.x, size.y, window_id);
                touch(manager, SDL_EVENT_FINGER_DOWN, 1, 300, 300, size.x, size.y, window_id);
                touch(manager, SDL_EVENT_FINGER_UP, 1, 250, 330, size.x, size.y, window_id);
                CHECK(manual.game().dir == LEFT);
            }
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
    test_no_navigation_devices(renderer, surface);
    CHECK(SDL_Init(SDL_INIT_GAMEPAD));
    test_gamepad_connection(renderer, surface);
    const SDL_JoystickID gamepad = attach_test_gamepad();
    test_account_selection(renderer, surface);
    test_ui_and_board(renderer, surface);
    CHECK(SDL_DetachVirtualJoystick(gamepad));
    if (!SDL_HasGamepad()) test_no_navigation_devices(renderer, surface);
    SDL_DestroyRenderer(renderer);
    SDL_DestroySurface(surface);
    SDL_Quit();
    test_window_backend();
    return TEST_RESULT();
}
