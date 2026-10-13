#include "manual_game_view.h"

#include "../renderer.h"

#include <iostream>

ManualGameView::ManualGameView(SessionState &session) : MenuView(MenuPage::Play), session_(session)
{
}

bool ManualGameView::owns(MenuPage candidate) const
{
    return candidate == MenuPage::Play || candidate == MenuPage::Difficulty;
}

std::string ManualGameView::title() const
{
    return page == MenuPage::Difficulty ? "CHOOSE DIFFICULTY" : "PLAY GAME (MANUAL MODE)";
}

std::vector<std::string> ManualGameView::details() const
{
    return {"Current game: 30x30, lethal walls, one apple, 100ms moves.",
            "Difficulty presets and obstacles are not implemented."};
}

std::vector<MenuItem> ManualGameView::items() const
{
    if (page == MenuPage::Difficulty)
        return {menu_unavailable("Easy (Slow Speed, No Obstacles)", page),
                menu_unavailable("Normal (Medium Speed, Standard Board)", page),
                menu_unavailable("Hard (Fast Speed, Dynamic Obstacles)", page),
                menu_link("Return to Play Game", MenuPage::Play)};
    return {menu_command("Start New Game", page, MenuAction::StartManualGame),
            menu_link("Choose Difficulty", MenuPage::Difficulty),
            menu_link("Return to Main Menu", MenuPage::Main)};
}

void ManualGameView::on_enter(ViewManager &manager)
{
    MenuView::on_enter(manager);
    playing_ = false;
}

void ManualGameView::handle_event(ViewManager &manager, const SDL_Event &event)
{
    if (!playing_)
    {
        MenuView::handle_event(manager, event);
        return;
    }
    if (event.type != SDL_EVENT_KEY_DOWN) return;

    const SDL_Keycode key = event.key.key;
    if (key == SDLK_ESCAPE)
    {
        playing_ = false;
        open(MenuPage::Play, "Manual game stopped.");
    }
    else if (key == SDLK_UP && game_.dir != DOWN)
        game_.dir = UP;
    else if (key == SDLK_DOWN && game_.dir != UP)
        game_.dir = DOWN;
    else if (key == SDLK_LEFT && game_.dir != RIGHT)
        game_.dir = LEFT;
    else if (key == SDLK_RIGHT && game_.dir != LEFT)
        game_.dir = RIGHT;
}

void ManualGameView::update(ViewManager &manager, Uint64 now_ms)
{
    (void)manager;
    if (!playing_ || !timer_.due(now_ms, MOVE_INTERVAL_MS)) return;

    game_.step(STRAIGHT);
    if (game_.game_over)
    {
        std::cout << "Game Over! Score: " << game_.score << std::endl;
        record_score(session_.best_manual_scores, game_.score);
        game_.reset();
    }
}

void ManualGameView::render(SDL_Renderer *renderer) const
{
    if (playing_)
        draw_game(renderer, game_);
    else
        MenuView::render(renderer);
}

bool ManualGameView::playing() const
{
    return playing_;
}

SnakeGame &ManualGameView::game()
{
    return game_;
}

void ManualGameView::perform(MenuAction action, ViewManager &manager)
{
    (void)manager;
    if (action != MenuAction::StartManualGame) return;
    playing_ = true;
    game_.reset();
    timer_.reset();
}
