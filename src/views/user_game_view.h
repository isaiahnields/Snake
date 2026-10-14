#pragma once

#include "../session_state.h"
#include "../snake_game.h"
#include "menu_view.h"

// Pre-game menu (Play Game / Choose Difficulty) and the user-controlled game itself.
class UserGameView : public MenuView
{
  public:
    static constexpr Uint64 MOVE_INTERVAL_MS = 100;

    explicit UserGameView(SessionState &session);

    bool owns(MenuPage candidate) const override;
    std::string title() const override;
    std::vector<std::string> details() const override;
    std::vector<MenuItem> items() const override;

    void on_enter(ViewManager &manager) override;
    void handle_event(ViewManager &manager, const SDL_Event &event) override;
    void update(ViewManager &manager, Uint64 now_ms) override;
    void render(SDL_Renderer *renderer) const override;

    bool playing() const;
    SnakeGame &game();

  protected:
    void perform(MenuAction action, ViewManager &manager) override;

  private:
    SessionState &session_;
    SnakeGame game_;
    StepTimer timer_;
    bool playing_ = false;
    bool swipe_active_ = false;
    bool swipe_handled_ = false;
    SDL_TouchID swipe_touch_ = 0;
    SDL_FingerID swipe_finger_ = 0;
    float swipe_x_ = 0;
    float swipe_y_ = 0;
    int swipe_width_ = WINDOW_WIDTH;
    int swipe_height_ = WINDOW_HEIGHT;

    void change_direction(Direction direction);
    void handle_touch(const SDL_Event &event);
};
