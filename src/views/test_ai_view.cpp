#include "test_ai_view.h"

#include "../renderer.h"
#include "../view_manager.h"

#include <iostream>

TestAIView::TestAIView(SessionState &session) : session_(session) {}

void TestAIView::on_enter(ViewManager &manager)
{
    (void)manager;
    game_.reset();
    timer_.reset();
}

void TestAIView::handle_event(ViewManager &manager, const SDL_Event &event)
{
    if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE)
        manager.open_page(MenuPage::Test, "Session stopped. DQN weights remain in memory.");
}

void TestAIView::update(ViewManager &manager, Uint64 now_ms)
{
    if (!timer_.due(now_ms, MOVE_INTERVAL_MS)) return;

    game_.step(session_.agent.get_action(game_.get_state_vector(), false));
    if (!game_.game_over) return;

    std::cout << "AI Game Over! Score: " << game_.score << std::endl;
    record_score(session_.best_ai_scores, game_.score);
    manager.open_page(MenuPage::Test, "AI test complete. Score: " + std::to_string(game_.score));
}

void TestAIView::render(SDL_Renderer *renderer) const
{
    draw_game(renderer, game_);
    draw_status(renderer,
                "Testing active AI  Score: " + std::to_string(game_.score) + "  Esc: Stop");
}
