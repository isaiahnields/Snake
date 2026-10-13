#pragma once

#include "../session_state.h"
#include "../snake_game.h"
#include "../view.h"

// Plays one visual game with the session DQN agent (no exploration) and reports its score.
class TestAIView : public View
{
  public:
    static constexpr Uint64 MOVE_INTERVAL_MS = 100;

    explicit TestAIView(SessionState &session);

    void on_enter(ViewManager &manager) override;
    void handle_event(ViewManager &manager, const SDL_Event &event) override;
    void update(ViewManager &manager, Uint64 now_ms) override;
    void render(SDL_Renderer *renderer) const override;

  private:
    SessionState &session_;
    SnakeGame game_;
    StepTimer timer_;
};
