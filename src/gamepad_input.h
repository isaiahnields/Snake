#pragma once

#include <SDL3/SDL.h>
#include <array>
#include <map>
#include <optional>
#include <vector>

class GamepadInput
{
  public:
    GamepadInput();
    ~GamepadInput();
    GamepadInput(const GamepadInput &) = delete;
    GamepadInput &operator=(const GamepadInput &) = delete;

    std::optional<SDL_Keycode> handle_event(const SDL_Event &event);
    std::vector<SDL_Gamepad *> gamepads() const;
    bool focused() const;

  private:
    struct State
    {
        SDL_Gamepad *gamepad = nullptr;
        std::array<bool, SDL_GAMEPAD_BUTTON_COUNT> buttons{};
        int x = 0;
        int y = 0;
        SDL_Keycode direction = 0;
    };

    std::map<SDL_JoystickID, State> devices_;
    bool focused_ = true;

    void open(SDL_JoystickID id);
    std::optional<SDL_Keycode> move_stick(State &state);
};
