#include "train_ai_view.h"

#include "../renderer.h"
#include "../view_manager.h"

#include <algorithm>
#include <cstdio>
#include <iostream>

namespace
{
std::string format_epsilon(float epsilon)
{
    char text[16];
    std::snprintf(text, sizeof(text), "%.3f", epsilon);
    return text;
}
} // namespace

TrainAIView::TrainAIView(SessionState &session) : session_(session) {}

void TrainAIView::on_enter(ViewManager &manager)
{
    (void)manager;
    game_.reset();
    episode_ = 1;
    last_score_ = 0;
    best_score_ = 0;
}

void TrainAIView::handle_event(ViewManager &manager, const SDL_Event &event)
{
    if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE)
        manager.open_page(MenuPage::Train, "Session stopped. DQN weights remain in memory.");
}

void TrainAIView::update(ViewManager &manager, Uint64 now_ms)
{
    (void)now_ms;
    if (session_.visual_training)
    {
        train_step(manager);
        return;
    }
    const Uint64 start_ms = SDL_GetTicks();
    while (train_step(manager) && SDL_GetTicks() - start_ms < HEADLESS_FRAME_BUDGET_MS)
    {
    }
}

bool TrainAIView::train_step(ViewManager &manager)
{
    DQNAgent &agent = session_.agent;
    const std::vector<float> state = game_.get_state_vector();
    const Action action = agent.get_action(state, true);
    const float reward = game_.step(action);
    const std::vector<float> next_state = game_.get_state_vector();

    agent.store_transition(state, action, reward, next_state, game_.game_over);
    agent.train();

    if (!game_.game_over) return true;

    agent.decay_epsilon();
    last_score_ = game_.score;
    best_score_ = std::max(best_score_, game_.score);
    if (episode_ % 50 == 0)
    {
        std::cout << "Episode: " << episode_ << " | Score: " << game_.score
                  << " | Epsilon: " << agent.epsilon << std::endl;
    }
    game_.reset();
    ++episode_;
    if (episode_ <= session_.training_episodes) return true;

    manager.open_page(MenuPage::Train,
                      "Training complete: " + std::to_string(session_.training_episodes) +
                          " episodes. Weights remain in memory.");
    return false;
}

void TrainAIView::render(SDL_Renderer *renderer) const
{
    const std::string progress =
        "Episode " + std::to_string(episode_) + "/" + std::to_string(session_.training_episodes);
    if (session_.visual_training)
    {
        draw_game(renderer, game_);
        draw_status(renderer, "Training  " + progress + "  Score: " + std::to_string(game_.score) +
                                  "  Epsilon: " + format_epsilon(session_.agent.epsilon) +
                                  "  Esc: Stop");
        return;
    }
    draw_info_screen(renderer, "TRAINING ACTIVE AI (HEADLESS)",
                     {progress, "Last score: " + std::to_string(last_score_),
                      "Best score: " + std::to_string(best_score_),
                      "Epsilon: " + format_epsilon(session_.agent.epsilon)},
                     "Esc: Stop training (learned weights remain in memory)");
}

int TrainAIView::frame_delay_ms() const
{
    return session_.visual_training ? DEFAULT_FRAME_DELAY_MS : 0;
}

int TrainAIView::episode() const
{
    return episode_;
}
