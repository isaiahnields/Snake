#include "test_helpers.h"

#include "../src/gamepad_input.h"
#include "../src/views/auth_form_view.h"
#include "../src/views/user_game_view.h"

#include <imgui.h>
#include <imgui_internal.h>

namespace
{
std::size_t connected_gamepad_count()
{
    int count = 0;
    SDL_JoystickID *ids = SDL_GetGamepads(&count);
    CHECK(ids != nullptr);
    SDL_free(ids);
    return static_cast<std::size_t>(count);
}

SDL_JoystickID attach_gamepad()
{
    SDL_VirtualJoystickDesc desc{};
    SDL_INIT_INTERFACE(&desc);
    desc.type = SDL_JOYSTICK_TYPE_GAMEPAD;
    desc.nbuttons = SDL_GAMEPAD_BUTTON_COUNT;
    desc.naxes = SDL_GAMEPAD_AXIS_COUNT;
    desc.name = "Controller input test";
    const SDL_JoystickID id = SDL_AttachVirtualJoystick(&desc);
    CHECK(id != 0);
    CHECK(SDL_IsGamepad(id));
    return id;
}

SDL_Event button_event(SDL_JoystickID id, Uint8 button, bool down = true)
{
    SDL_Event event{};
    event.type = down ? SDL_EVENT_GAMEPAD_BUTTON_DOWN : SDL_EVENT_GAMEPAD_BUTTON_UP;
    event.gbutton.which = id;
    event.gbutton.button = button;
    event.gbutton.down = down;
    return event;
}

SDL_Event axis_event(SDL_JoystickID id, Uint8 axis, Sint16 value)
{
    SDL_Event event{};
    event.type = SDL_EVENT_GAMEPAD_AXIS_MOTION;
    event.gaxis.which = id;
    event.gaxis.axis = axis;
    event.gaxis.value = value;
    return event;
}

void drain_events(ViewManager &manager)
{
    SDL_Event event{};
    while (SDL_PollEvent(&event))
        manager.handle_event(event);
}

void press(ViewManager &manager, SDL_JoystickID id, SDL_GamepadButton button)
{
    SDL_Gamepad *gamepad = SDL_GetGamepadFromID(id);
    CHECK(gamepad != nullptr);
    if (gamepad == nullptr) return;
    SDL_Joystick *joystick = SDL_GetGamepadJoystick(gamepad);
    CHECK(SDL_SetJoystickVirtualButton(joystick, button, true));
    drain_events(manager);
    CHECK(SDL_SetJoystickVirtualButton(joystick, button, false));
    drain_events(manager);
}

void move_stick(ViewManager &manager, SDL_JoystickID id, SDL_GamepadAxis axis, Sint16 value)
{
    SDL_Gamepad *gamepad = SDL_GetGamepadFromID(id);
    CHECK(gamepad != nullptr);
    if (gamepad == nullptr) return;
    CHECK(SDL_SetJoystickVirtualAxis(SDL_GetGamepadJoystick(gamepad), axis, value));
    drain_events(manager);
}

void test_lifetime_and_mapping()
{
    const auto existing_count = connected_gamepad_count();
    const SDL_JoystickID id = attach_gamepad();
    {
        GamepadInput input;
        CHECK(input.gamepads().size() == existing_count + 1);
        CHECK(SDL_GetGamepadFromID(id) != nullptr);
        SDL_Event added{};
        added.type = SDL_EVENT_GAMEPAD_ADDED;
        added.gdevice.which = id;
        CHECK(!input.handle_event(added));
        CHECK(input.gamepads().size() == existing_count + 1);

        const std::pair<SDL_GamepadButton, SDL_Keycode> bindings[] = {
            {SDL_GAMEPAD_BUTTON_DPAD_UP, SDLK_UP},     {SDL_GAMEPAD_BUTTON_DPAD_DOWN, SDLK_DOWN},
            {SDL_GAMEPAD_BUTTON_DPAD_LEFT, SDLK_LEFT}, {SDL_GAMEPAD_BUTTON_DPAD_RIGHT, SDLK_RIGHT},
            {SDL_GAMEPAD_BUTTON_SOUTH, SDLK_RETURN},   {SDL_GAMEPAD_BUTTON_EAST, SDLK_ESCAPE},
            {SDL_GAMEPAD_BUTTON_START, SDLK_ESCAPE},   {SDL_GAMEPAD_BUTTON_BACK, SDLK_ESCAPE}};
        for (const auto &binding : bindings)
        {
            CHECK(input.handle_event(button_event(id, binding.first)) == binding.second);
            CHECK(!input.handle_event(button_event(id, binding.first)));
            CHECK(!input.handle_event(button_event(id, binding.first, false)));
            CHECK(input.handle_event(button_event(id, binding.first)) == binding.second);
            CHECK(!input.handle_event(button_event(id, binding.first, false)));
        }
        CHECK(!input.handle_event(button_event(id, SDL_GAMEPAD_BUTTON_NORTH)));
        CHECK(!input.handle_event(button_event(id, SDL_GAMEPAD_BUTTON_WEST)));
        CHECK(!input.handle_event(button_event(id, SDL_GAMEPAD_BUTTON_COUNT)));
        CHECK(!input.handle_event(button_event(0, SDL_GAMEPAD_BUTTON_SOUTH)));
    }
    // Processing the initial ADDED event must not open a second reference.
    CHECK(SDL_GetGamepadFromID(id) == nullptr);
    CHECK(SDL_DetachVirtualJoystick(id));
    SDL_FlushEvents(SDL_EVENT_FIRST, SDL_EVENT_LAST);
}

void test_stick_and_focus()
{
    const auto existing_count = connected_gamepad_count();
    const SDL_JoystickID first = attach_gamepad();
    const SDL_JoystickID second = attach_gamepad();
    {
        GamepadInput input;
        CHECK(input.gamepads().size() == existing_count + 2);
        const auto axis = [&](SDL_JoystickID id, SDL_GamepadAxis which, Sint16 value)
        { return input.handle_event(axis_event(id, which, value)); };
        CHECK(!axis(first, SDL_GAMEPAD_AXIS_LEFTY, 15999));
        CHECK(axis(first, SDL_GAMEPAD_AXIS_LEFTY, 16000) == SDLK_DOWN);
        CHECK(!axis(first, SDL_GAMEPAD_AXIS_LEFTY, 32767));
        CHECK(!axis(first, SDL_GAMEPAD_AXIS_LEFTY, 8001));
        CHECK(!axis(first, SDL_GAMEPAD_AXIS_LEFTY, 16000));
        CHECK(!axis(first, SDL_GAMEPAD_AXIS_LEFTY, 8000));
        CHECK(axis(first, SDL_GAMEPAD_AXIS_LEFTY, 16000) == SDLK_DOWN);
        CHECK(axis(second, SDL_GAMEPAD_AXIS_LEFTY, 16000) == SDLK_DOWN);
        CHECK(!axis(first, SDL_GAMEPAD_AXIS_LEFTY, 0));
        CHECK(axis(first, SDL_GAMEPAD_AXIS_LEFTY, -32768) == SDLK_UP);
        CHECK(!axis(first, SDL_GAMEPAD_AXIS_LEFTY, 0));
        CHECK(axis(first, SDL_GAMEPAD_AXIS_LEFTX, 20000) == SDLK_RIGHT);
        CHECK(!axis(first, SDL_GAMEPAD_AXIS_LEFTY, -21000));
        CHECK(axis(first, SDL_GAMEPAD_AXIS_LEFTY, -26000) == SDLK_UP);
        CHECK(!axis(first, SDL_GAMEPAD_AXIS_LEFTX, 0));
        CHECK(!axis(first, SDL_GAMEPAD_AXIS_LEFTY, 0));
        CHECK(axis(first, SDL_GAMEPAD_AXIS_LEFTX, -32768) == SDLK_LEFT);
        CHECK(!axis(first, SDL_GAMEPAD_AXIS_RIGHTX, 32767));
        CHECK(!axis(first, SDL_GAMEPAD_AXIS_LEFT_TRIGGER, 32767));
        CHECK(!axis(0, SDL_GAMEPAD_AXIS_LEFTY, 32767));

        for (const auto type : {SDL_EVENT_WINDOW_FOCUS_LOST, SDL_EVENT_DID_ENTER_BACKGROUND})
        {
            SDL_Event focus{};
            focus.type = type;
            CHECK(!input.handle_event(focus));
            CHECK(!input.focused());
            CHECK(!input.handle_event(button_event(first, SDL_GAMEPAD_BUTTON_SOUTH)));
            CHECK(!axis(first, SDL_GAMEPAD_AXIS_LEFTY, -32768));
            focus.type = type == SDL_EVENT_WINDOW_FOCUS_LOST ? SDL_EVENT_WINDOW_FOCUS_GAINED
                                                             : SDL_EVENT_DID_ENTER_FOREGROUND;
            CHECK(!input.handle_event(focus));
            CHECK(input.focused());
            CHECK(axis(first, SDL_GAMEPAD_AXIS_LEFTY, -32768) == SDLK_UP);
        }

        SDL_Event removed{};
        removed.type = SDL_EVENT_GAMEPAD_REMOVED;
        removed.gdevice.which = first;
        CHECK(!input.handle_event(removed));
        CHECK(input.gamepads().size() == existing_count + 1);
        CHECK(SDL_GetGamepadFromID(first) == nullptr);
        CHECK(!input.handle_event(button_event(first, SDL_GAMEPAD_BUTTON_SOUTH)));
    }
    CHECK(SDL_DetachVirtualJoystick(first));
    CHECK(SDL_DetachVirtualJoystick(second));
    SDL_FlushEvents(SDL_EVENT_FIRST, SDL_EVENT_LAST);
}

void test_menus_and_game()
{
    ViewManager manager;
    authenticate_test_player(manager);
    const SDL_JoystickID id = attach_gamepad();
    drain_events(manager);
    CHECK(SDL_GetGamepadFromID(id) != nullptr);
    MenuView &menu = *manager.active().as_menu();
    move_stick(manager, id, SDL_GAMEPAD_AXIS_LEFTY, 5000);
    CHECK(menu.selection == 0);
    move_stick(manager, id, SDL_GAMEPAD_AXIS_LEFTY, 16000);
    CHECK(menu.selection == 1);
    move_stick(manager, id, SDL_GAMEPAD_AXIS_LEFTY, 32767);
    CHECK(menu.selection == 1);
    move_stick(manager, id, SDL_GAMEPAD_AXIS_LEFTY, 0);
    move_stick(manager, id, SDL_GAMEPAD_AXIS_LEFTY, -32768);
    CHECK(menu.selection == 0);
    move_stick(manager, id, SDL_GAMEPAD_AXIS_LEFTY, 0);
    press(manager, id, SDL_GAMEPAD_BUTTON_DPAD_UP);
    CHECK(menu.selection == 6);
    press(manager, id, SDL_GAMEPAD_BUTTON_DPAD_DOWN);
    CHECK(menu.selection == 0);
    press(manager, id, SDL_GAMEPAD_BUTTON_SOUTH);
    CHECK(manager.active_id() == ViewId::UserGame);
    press(manager, id, SDL_GAMEPAD_BUTTON_SOUTH);
    auto &manual = static_cast<UserGameView &>(manager.active());
    CHECK(manual.playing());
    CHECK(manual.game().dir == RIGHT);
    press(manager, id, SDL_GAMEPAD_BUTTON_DPAD_LEFT);
    CHECK(manual.game().dir == RIGHT);
    press(manager, id, SDL_GAMEPAD_BUTTON_DPAD_UP);
    CHECK(manual.game().dir == UP);
    press(manager, id, SDL_GAMEPAD_BUTTON_DPAD_LEFT);
    CHECK(manual.game().dir == LEFT);
    press(manager, id, SDL_GAMEPAD_BUTTON_DPAD_DOWN);
    CHECK(manual.game().dir == DOWN);
    press(manager, id, SDL_GAMEPAD_BUTTON_DPAD_RIGHT);
    CHECK(manual.game().dir == RIGHT);

    move_stick(manager, id, SDL_GAMEPAD_AXIS_LEFTY, -32768);
    CHECK(manual.game().dir == UP);
    move_stick(manager, id, SDL_GAMEPAD_AXIS_LEFTY, 0);
    move_stick(manager, id, SDL_GAMEPAD_AXIS_LEFTX, -32768);
    CHECK(manual.game().dir == LEFT);
    press(manager, id, SDL_GAMEPAD_BUTTON_START);
    CHECK(!manual.playing());
    CHECK(manual.notice == "Manual game stopped.");
    press(manager, id, SDL_GAMEPAD_BUTTON_EAST);
    CHECK(manager.active_id() == ViewId::MainMenu);
    press(manager, id, SDL_GAMEPAD_BUTTON_BACK);
    CHECK(manager.active_id() == ViewId::SaveAndExit);
    press(manager, id, SDL_GAMEPAD_BUTTON_EAST);
    CHECK(manager.active_id() == ViewId::MainMenu);
    CHECK(menu.selection == 0);

    CHECK(SDL_DetachVirtualJoystick(id));
    drain_events(manager);
    CHECK(SDL_GetGamepadFromID(id) == nullptr);
    CHECK(menu.selection == 0);
    manager.handle_event(button_event(id, SDL_GAMEPAD_BUTTON_SOUTH));
    CHECK(manager.active_id() == ViewId::MainMenu);
    const SDL_JoystickID replacement = attach_gamepad();
    drain_events(manager);
    CHECK(SDL_GetGamepadFromID(replacement) != nullptr);
    CHECK(menu.selection == 0);
    press(manager, replacement, SDL_GAMEPAD_BUTTON_SOUTH);
    CHECK(manager.active_id() == ViewId::UserGame);
    press(manager, replacement, SDL_GAMEPAD_BUTTON_EAST);
    CHECK(manager.active_id() == ViewId::MainMenu);
    CHECK(SDL_DetachVirtualJoystick(replacement));
    drain_events(manager);
}

void render_frames(ViewManager &manager)
{
    for (int frame = 0; frame < 3; ++frame)
    {
        drain_events(manager);
        CHECK(manager.render());
    }
}

void ui_press(ViewManager &manager, SDL_JoystickID id, SDL_GamepadButton button)
{
    SDL_Gamepad *gamepad = SDL_GetGamepadFromID(id);
    CHECK(gamepad != nullptr);
    if (gamepad == nullptr) return;
    SDL_Joystick *joystick = SDL_GetGamepadJoystick(gamepad);
    CHECK(SDL_SetJoystickVirtualButton(joystick, button, true));
    render_frames(manager);
    CHECK(SDL_SetJoystickVirtualButton(joystick, button, false));
    render_frames(manager);
}

void test_rendered_controller_game(SDL_Renderer *renderer)
{
    ViewManager manager(renderer);
    authenticate_test_player(manager);
    CHECK(manager.initialize_ui());
    render_frames(manager);
    const SDL_JoystickID first = attach_gamepad();
    render_frames(manager);

    ui_press(manager, first, SDL_GAMEPAD_BUTTON_SOUTH);
    CHECK(manager.active_id() == ViewId::UserGame);
    ui_press(manager, first, SDL_GAMEPAD_BUTTON_SOUTH);
    auto &manual = static_cast<UserGameView &>(manager.view(ViewId::UserGame));
    CHECK(manual.playing());
    CHECK((ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_NavEnableGamepad) == 0);

    const Point start = manual.game().snake.front();
    ui_press(manager, first, SDL_GAMEPAD_BUTTON_DPAD_UP);
    manual.update(manager, 0);
    manual.update(manager, UserGameView::MOVE_INTERVAL_MS);
    CHECK((manual.game().snake.front() == Point{start.x, start.y - 1}));

    const SDL_JoystickID second = attach_gamepad();
    render_frames(manager);
    move_stick(manager, second, SDL_GAMEPAD_AXIS_LEFTX, -32768);
    render_frames(manager);
    manual.update(manager, 2 * UserGameView::MOVE_INTERVAL_MS);
    CHECK((manual.game().snake.front() == Point{start.x - 1, start.y - 1}));

    CHECK(SDL_DetachVirtualJoystick(first));
    render_frames(manager);
    CHECK(manual.playing());
    ui_press(manager, second, SDL_GAMEPAD_BUTTON_EAST);
    CHECK(!manual.playing());
    CHECK(manual.page == MenuPage::Play);
    ui_press(manager, second, SDL_GAMEPAD_BUTTON_EAST);
    CHECK(manager.active_id() == ViewId::MainMenu);
    CHECK(SDL_DetachVirtualJoystick(second));
    render_frames(manager);
}

void test_controller_menu_pages(SDL_Renderer *renderer)
{
    const SDL_JoystickID id = attach_gamepad();
    ViewManager manager(renderer);
    authenticate_test_player(manager);
    CHECK(manager.initialize_ui());
    for (int value = static_cast<int>(MenuPage::Main); value <= static_cast<int>(MenuPage::Exit);
         ++value)
    {
        const auto page = static_cast<MenuPage>(value);
        manager.open_page(page);
        render_frames(manager);
        MenuView *menu = manager.active().as_menu();
        CHECK(menu != nullptr);
        if (menu == nullptr) continue;
        const auto entries = menu->items();
        ui_press(manager, id, SDL_GAMEPAD_BUTTON_DPAD_UP);
        CHECK(menu->selection == static_cast<int>(entries.size()) - 1);
        ui_press(manager, id, SDL_GAMEPAD_BUTTON_DPAD_DOWN);
        CHECK(menu->selection == 0);
        // Every menu's last item returns to its parent.
        ui_press(manager, id, SDL_GAMEPAD_BUTTON_EAST);
        CHECK(manager.active().as_menu() != nullptr);
        CHECK(manager.active().as_menu()->page == entries.back().target);
        manager.open_page(page);
        render_frames(manager);
        ui_press(manager, id, SDL_GAMEPAD_BUTTON_DPAD_UP);
        ui_press(manager, id, SDL_GAMEPAD_BUTTON_SOUTH);
        CHECK(manager.active().as_menu() != nullptr);
        CHECK(manager.active().as_menu()->page == entries.back().target);
    }
    CHECK(SDL_DetachVirtualJoystick(id));
    render_frames(manager);
}

void test_account_forms(SDL_Renderer *renderer)
{
    const SDL_JoystickID id = attach_gamepad();
    {
        ViewManager manager(renderer);
        CHECK(manager.initialize_ui());
        render_frames(manager);
        ui_press(manager, id, SDL_GAMEPAD_BUTTON_DPAD_DOWN);
        ui_press(manager, id, SDL_GAMEPAD_BUTTON_SOUTH);
        CHECK(manager.active_id() == ViewId::SignIn);
        CHECK(static_cast<AuthFormView &>(manager.active()).notice().empty());
        CHECK((ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_NavEnableGamepad) != 0);

        // Navigate to a non-submit action and activate it through the SDL backend.
        ImGuiWindow *window = ImGui::FindWindowByName("Snake UI");
        CHECK(window != nullptr);
        if (window != nullptr)
        {
            const ImGuiID forgot_password = window->GetID("Forgot Password");
            for (int step = 0; step < 8 && ImGui::GetCurrentContext()->NavId != forgot_password;
                 ++step)
            {
                if (step % 2 == 0)
                {
                    move_stick(manager, id, SDL_GAMEPAD_AXIS_LEFTY, 32767);
                    render_frames(manager);
                    move_stick(manager, id, SDL_GAMEPAD_AXIS_LEFTY, 0);
                    render_frames(manager);
                }
                else
                    ui_press(manager, id, SDL_GAMEPAD_BUTTON_DPAD_DOWN);
            }
            CHECK(ImGui::GetCurrentContext()->NavId == forgot_password);
            ui_press(manager, id, SDL_GAMEPAD_BUTTON_SOUTH);
            CHECK(manager.active_id() == ViewId::ForgotPassword);
        }
        ui_press(manager, id, SDL_GAMEPAD_BUTTON_EAST);
        CHECK(manager.active_id() == ViewId::SignIn);
        ui_press(manager, id, SDL_GAMEPAD_BUTTON_EAST);
        CHECK(manager.active_id() == ViewId::SignUpOrSignIn);
        CHECK((ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_NavEnableGamepad) == 0);

        authenticate_test_player(manager);
        render_frames(manager);
        ui_press(manager, id, SDL_GAMEPAD_BUTTON_DPAD_DOWN);
        CHECK(manager.active().as_menu()->selection == 1);
        ui_press(manager, id, SDL_GAMEPAD_BUTTON_SOUTH);
        CHECK(manager.active_id() == ViewId::AIControlCenter);
        CHECK(manager.active().as_menu()->page == MenuPage::AI);
        ui_press(manager, id, SDL_GAMEPAD_BUTTON_EAST);
        CHECK(manager.active_id() == ViewId::MainMenu);
        CHECK(SDL_DetachVirtualJoystick(id));
        render_frames(manager);
    }
    CHECK(SDL_GetGamepadFromID(id) == nullptr);
}
} // namespace

int main()
{
    CHECK(SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy"));
    CHECK(SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1"));
    CHECK(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD));
    test_lifetime_and_mapping();
    test_stick_and_focus();
    test_menus_and_game();
    SDL_Window *window = nullptr;
    SDL_Renderer *renderer = nullptr;
    CHECK(SDL_CreateWindowAndRenderer("Controller test", WINDOW_WIDTH, WINDOW_HEIGHT,
                                      SDL_WINDOW_HIDDEN, &window, &renderer));
    if (renderer != nullptr)
    {
        test_rendered_controller_game(renderer);
        test_controller_menu_pages(renderer);
        test_account_forms(renderer);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
    }
    SDL_Quit();
    return TEST_RESULT();
}
