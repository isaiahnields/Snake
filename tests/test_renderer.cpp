#include "test_helpers.h"

#include "../src/renderer.h"
#include "../src/view_manager.h"
#include "../src/views/user_game_view.h"

#include <SDL3/SDL.h>
#include <algorithm>
#include <cmath>
#include <imgui.h>
#include <utility>

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
    game.board.snake().body = {{2, 2}, {1, 2}};
    game.board.place_food_at({10, 10});
    manager.render();
    const GameLayout layout = game_layout(WINDOW_WIDTH, WINDOW_HEIGHT);
    const auto cell_center = [&](int x, int y)
    {
        return Point{static_cast<int>(layout.board.x + (x + 0.5f) * layout.cell_size),
                     static_cast<int>(layout.board.y + (y + 0.5f) * layout.cell_size)};
    };
    const Point food = cell_center(10, 10), head = cell_center(2, 2), tail = cell_center(1, 2);
    const Point empty = cell_center(20, 20);
    CHECK(pixel_is(surface, food.x, food.y, 255, 50, 50));
    CHECK(pixel_is(surface, head.x, head.y, 50, 255, 50));
    CHECK(pixel_is(surface, tail.x, tail.y, 30, 150, 30));
    CHECK(pixel_is(surface, empty.x, empty.y, 18, 18, 18));
    // Out-of-bounds space is separated from the board by a clear border.
    const int left_border = static_cast<int>(layout.board.x - layout.border / 2);
    const int bottom_border = static_cast<int>(layout.board.y + layout.board.h + layout.border / 2);
    CHECK(pixel_is(surface, left_border, empty.y, 220, 220, 220));
    CHECK(pixel_is(surface, empty.x, bottom_border, 220, 220, 220));
    CHECK(pixel_is(surface, static_cast<int>(layout.board.x - layout.border) - 2, empty.y, 30, 30,
                   30));
    CHECK(pixel_is(surface, 2, WINDOW_HEIGHT - 2, 30, 30, 30));

    // The HUD shows the score and offers mute and exit buttons.
    const auto score_text_right = [&]()
    {
        int right = -1;
        const int top = static_cast<int>(layout.hud.y), bottom = static_cast<int>(layout.hud.h);
        for (int x = 0; x < static_cast<int>(layout.mute_button.x); ++x)
            for (int y = top; y < bottom; ++y)
                if (!pixel_is(surface, x, y, 20, 20, 20)) right = x;
        return right;
    };
    CHECK(pixel_is(surface, 2, 2, 20, 20, 20));
    game.score = 7;
    manager.render();
    const int short_score = score_text_right();
    CHECK(short_score > 0);
    game.score = 12345;
    manager.render();
    CHECK(score_text_right() > short_score);
    CHECK(manual.playing());
    CHECK(!manager.session().muted);
    const auto button_center = [](const SDL_FRect &rect)
    { return std::pair<float, float>{rect.x + rect.w / 2, rect.y + rect.h / 2}; };
    const auto mute = button_center(layout.mute_button);
    CHECK(pixel_is(surface, static_cast<int>(mute.first),
                   static_cast<int>(layout.mute_button.y + 2), 45, 45, 45));
    click(manager, mute.first, mute.second);
    CHECK(manager.session().muted);
    CHECK(manual.playing());
    click(manager, mute.first, mute.second);
    CHECK(!manager.session().muted);
    CHECK(manual.playing());
    const auto exit = button_center(layout.exit_button);
    click(manager, exit.first, exit.second);
    CHECK(!manual.playing());
    CHECK(manual.page == MenuPage::Play);
    CHECK(manager.active_id() == ViewId::UserGame);

    // Game Over pop-up shows the final score with New Game and Main Menu buttons.
    const auto finish_game = [&]()
    {
        Uint64 now_ms = 0;
        for (int update = 0; update < 100 && !manual.game_over(); ++update)
        {
            manual.update(manager, now_ms);
            now_ms += UserGameView::MOVE_INTERVAL_MS;
        }
        CHECK(manual.game_over());
        manager.render();
    };
    press(manager, SDLK_RETURN);
    CHECK(manual.playing());
    finish_game();
    const SDL_FRect &popup = layout.game_over_popup;
    CHECK(pixel_is(surface, static_cast<int>(popup.x + popup.w / 2),
                   static_cast<int>(popup.y + layout.border + 4), 20, 20, 20));
    // The board behind the pop-up is dimmed.
    CHECK(!pixel_is(surface, empty.x, empty.y, 18, 18, 18));
    const auto new_game = button_center(layout.new_game_button);
    click(manager, new_game.first, new_game.second);
    CHECK(manual.playing());
    CHECK(!manual.game_over());
    CHECK(manager.active_id() == ViewId::UserGame);
    finish_game();
    const auto main_menu = button_center(layout.main_menu_button);
    click(manager, main_menu.first, main_menu.second);
    CHECK(!manual.playing());
    CHECK(manager.active_id() == ViewId::MainMenu);

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
                manual.game().board.snake().dir = RIGHT;
                touch(manager, SDL_EVENT_FINGER_DOWN, 1, 300, 300, size.x, size.y, window_id);
                touch(manager, SDL_EVENT_FINGER_MOTION, 1, 300, 280, size.x, size.y, window_id);
                CHECK(manual.game().board.snake().dir == RIGHT);
                touch(manager, SDL_EVENT_FINGER_MOTION, 1, 330, 250, size.x, size.y, window_id);
                CHECK(manual.game().board.snake().dir == UP);
                touch(manager, SDL_EVENT_FINGER_UP, 1, 330, 250, size.x, size.y, window_id);
                touch(manager, SDL_EVENT_FINGER_DOWN, 1, 300, 300, size.x, size.y, window_id);
                touch(manager, SDL_EVENT_FINGER_UP, 1, 250, 330, size.x, size.y, window_id);
                CHECK(manual.game().board.snake().dir == LEFT);
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

static void test_game_layout()
{
    float previous_cell = 0;
    for (const auto &size : {Point{MIN_WINDOW_WIDTH, MIN_WINDOW_HEIGHT},
                             Point{WINDOW_WIDTH, WINDOW_HEIGHT}, Point{1200, 600}, Point{600, 1200},
                             Point{1920, 1080}, Point{2400, 2400}, Point{801, 603}})
    {
        const float width = static_cast<float>(size.x), height = static_cast<float>(size.y);
        const GameLayout layout = game_layout(width, height);
        const SDL_FRect &board = layout.board;
        CHECK(layout.cell_size >= 1);
        CHECK(board.w == layout.cell_size * GRID_WIDTH &&
              board.h == layout.cell_size * GRID_HEIGHT);
        const float hud_bottom = layout.hud.y + layout.hud.h;
        // Centered in the space below the HUD, with the whole bordered board visible.
        CHECK(std::abs(board.x + board.w / 2 - width / 2) <= 1);
        CHECK(std::abs(board.y + board.h / 2 - (hud_bottom + height) / 2) <= 0.5f);
        const float unit = std::min(width, height) / WINDOW_WIDTH;
        const float margin = std::max(8.0f, std::round(16.0f * unit));
        const float available_width = width - 2 * (margin + layout.border);
        const float available_height = height - hud_bottom - 2 * (margin + layout.border);
        CHECK(layout.cell_size ==
              std::floor(std::min(available_width / GRID_WIDTH, available_height / GRID_HEIGHT)));
        CHECK(layout.border >= 2);
        CHECK(board.x - layout.border >= 0 && board.x + board.w + layout.border <= width);
        CHECK(board.y - layout.border > layout.hud.y + layout.hud.h);
        CHECK(board.y + board.h + layout.border <= height);
        CHECK(layout.hud.w == width && layout.hud.h >= 36);
        for (const SDL_FRect &button : {layout.mute_button, layout.exit_button})
        {
            CHECK(button.x > 0 && button.x + button.w < width);
            CHECK(button.y >= 0 && button.y + button.h <= layout.hud.h);
        }
        CHECK(layout.mute_button.x + layout.mute_button.w < layout.exit_button.x);
        if (size.x == size.y)
        {
            CHECK(layout.cell_size > previous_cell);
            previous_cell = layout.cell_size;
        }
    }
}

int main()
{
    test_game_layout();
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
