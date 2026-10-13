#pragma once

#include "../session_state.h"
#include "../snake_game.h"
#include "menu_view.h"

// Pre-game menu (Play Game / Choose Difficulty) and the manually controlled game itself.
class ManualGameView : public MenuView
{
  public:
    static constexpr Uint64 MOVE_INTERVAL_MS = 100;

    explicit ManualGameView(SessionState &session);

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
};
