#include "gamepad_input.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>

namespace
{
constexpr int STICK_PRESS_THRESHOLD = 16000;
constexpr int STICK_RELEASE_THRESHOLD = 8000;
constexpr int STICK_AXIS_HYSTERESIS = 4000;

std::optional<SDL_Keycode> button_key(Uint8 button)
{
    switch (button)
    {
    case SDL_GAMEPAD_BUTTON_DPAD_UP:
        return SDLK_UP;
    case SDL_GAMEPAD_BUTTON_DPAD_DOWN:
        return SDLK_DOWN;
    case SDL_GAMEPAD_BUTTON_DPAD_LEFT:
        return SDLK_LEFT;
    case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:
        return SDLK_RIGHT;
    case SDL_GAMEPAD_BUTTON_SOUTH:
        return SDLK_RETURN;
    case SDL_GAMEPAD_BUTTON_EAST:
    case SDL_GAMEPAD_BUTTON_START:
    case SDL_GAMEPAD_BUTTON_BACK:
        return SDLK_ESCAPE;
    default:
        return std::nullopt;
    }
}
} // namespace

GamepadInput::GamepadInput()
{
    if (SDL_WasInit(SDL_INIT_GAMEPAD) == 0) return;
    int count = 0;
    SDL_JoystickID *ids = SDL_GetGamepads(&count);
    if (ids == nullptr)
    {
        std::cerr << "Gamepad enumeration error: " << SDL_GetError() << std::endl;
        return;
    }
    for (int index = 0; index < count; ++index)
        open(ids[index]);
    SDL_free(ids);
}

GamepadInput::~GamepadInput()
{
    for (const auto &device : devices_)
        SDL_CloseGamepad(device.second.gamepad);
}

void GamepadInput::open(SDL_JoystickID id)
{
    if (devices_.find(id) != devices_.end()) return;
    SDL_Gamepad *gamepad = SDL_OpenGamepad(id);
    if (gamepad == nullptr)
    {
        std::cerr << "Gamepad open error (" << id << "): " << SDL_GetError() << std::endl;
        return;
    }
    State state;
    state.gamepad = gamepad;
    devices_.emplace(id, state);
}

std::vector<SDL_Gamepad *> GamepadInput::gamepads() const
{
    std::vector<SDL_Gamepad *> result;
    for (const auto &device : devices_)
        result.push_back(device.second.gamepad);
    return result;
}

bool GamepadInput::focused() const
{
    return focused_;
}

std::optional<SDL_Keycode> GamepadInput::move_stick(State &state)
{
    const int x = std::abs(state.x);
    const int y = std::abs(state.y);
    if (std::max(x, y) <= STICK_RELEASE_THRESHOLD)
    {
        state.direction = 0;
        return std::nullopt;
    }
    if (std::max(x, y) < STICK_PRESS_THRESHOLD) return std::nullopt;

    bool horizontal = x > y;
    // Keep the current axis near a diagonal to avoid jitter causing extra turns.
    if (state.direction != 0 && std::abs(x - y) < STICK_AXIS_HYSTERESIS)
        horizontal = state.direction == SDLK_LEFT || state.direction == SDLK_RIGHT;
    const SDL_Keycode direction =
        horizontal ? (state.x < 0 ? SDLK_LEFT : SDLK_RIGHT) : (state.y < 0 ? SDLK_UP : SDLK_DOWN);
    if (direction == state.direction) return std::nullopt;
    state.direction = direction;
    return direction;
}

std::optional<SDL_Keycode> GamepadInput::handle_event(const SDL_Event &event)
{
    if (event.type == SDL_EVENT_GAMEPAD_ADDED)
    {
        open(event.gdevice.which);
        return std::nullopt;
    }
    if (event.type == SDL_EVENT_GAMEPAD_REMOVED)
    {
        const auto device = devices_.find(event.gdevice.which);
        if (device != devices_.end())
        {
            SDL_CloseGamepad(device->second.gamepad);
            devices_.erase(device);
        }
        return std::nullopt;
    }
    if (event.type == SDL_EVENT_WINDOW_FOCUS_LOST || event.type == SDL_EVENT_DID_ENTER_BACKGROUND)
    {
        focused_ = false;
        for (auto &device : devices_)
        {
            device.second.buttons.fill(false);
            device.second.x = device.second.y = 0;
            device.second.direction = 0;
        }
    }
    else if (event.type == SDL_EVENT_WINDOW_FOCUS_GAINED ||
             event.type == SDL_EVENT_DID_ENTER_FOREGROUND)
        focused_ = true;

    if (!focused_) return std::nullopt;
    if (event.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN || event.type == SDL_EVENT_GAMEPAD_BUTTON_UP)
    {
        const auto device = devices_.find(event.gbutton.which);
        if (device == devices_.end() || !SDL_GamepadConnected(device->second.gamepad) ||
            event.gbutton.button >= SDL_GAMEPAD_BUTTON_COUNT)
            return std::nullopt;
        bool &held = device->second.buttons[event.gbutton.button];
        const bool down = event.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN;
        const bool pressed = down && !held;
        held = down;
        return pressed ? button_key(event.gbutton.button) : std::nullopt;
    }
    if (event.type != SDL_EVENT_GAMEPAD_AXIS_MOTION ||
        (event.gaxis.axis != SDL_GAMEPAD_AXIS_LEFTX && event.gaxis.axis != SDL_GAMEPAD_AXIS_LEFTY))
        return std::nullopt;
    const auto device = devices_.find(event.gaxis.which);
    if (device == devices_.end() || !SDL_GamepadConnected(device->second.gamepad))
        return std::nullopt;
    State &state = device->second;
    if (event.gaxis.axis == SDL_GAMEPAD_AXIS_LEFTX)
        state.x = event.gaxis.value;
    else
        state.y = event.gaxis.value;
    return move_stick(state);
}
