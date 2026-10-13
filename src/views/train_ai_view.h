#pragma once

#include "../session_state.h"
#include "../snake_game.h"
#include "../view.h"

// Runs a training session for the session DQN agent and shows its progress, either by
// rendering every move (visual mode) or a statistics screen (headless mode).
class TrainAIView : public View
{
  public:
    static constexpr Uint64 HEADLESS_FRAME_BUDGET_MS = 15;

    explicit TrainAIView(SessionState &session);

    void on_enter(ViewManager &manager) override;
    void handle_event(ViewManager &manager, const SDL_Event &event) override;
    void update(ViewManager &manager, Uint64 now_ms) override;
    void render(SDL_Renderer *renderer) const override;
    int frame_delay_ms() const override;

    int episode() const;

  private:
    // Advances training by one move; returns false once the session has finished.
    bool train_step(ViewManager &manager);

    SessionState &session_;
    SnakeGame game_;
    int episode_ = 1;
    int last_score_ = 0;
    int best_score_ = 0;
};
