#include "user_game_view.h"

#include "../renderer.h"
#include "../view_manager.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <utility>

UserGameView::UserGameView(SessionState &session) : MenuView(MenuPage::Play), session_(session) {}

bool UserGameView::owns(MenuPage candidate) const
{
    return candidate == MenuPage::Play || candidate == MenuPage::Difficulty;
}

std::string UserGameView::title() const
{
    return page == MenuPage::Difficulty ? "CHOOSE DIFFICULTY" : "PLAY GAME (MANUAL MODE)";
}

std::vector<std::string> UserGameView::details() const
{
    return {"Current game: 30x30, lethal walls, one apple, 100ms moves.",
            "Controller: D-pad/left stick to steer; B/Circle or Start to stop.",
            "Difficulty presets and obstacles are not implemented."};
}

std::vector<MenuItem> UserGameView::items() const
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

void UserGameView::on_enter(ViewManager &manager)
{
    MenuView::on_enter(manager);
    playing_ = false;
    game_over_ = false;
    swipe_active_ = false;
}

void UserGameView::handle_event(ViewManager &manager, const SDL_Event &event)
{
    if (!playing_)
    {
        MenuView::handle_event(manager, event);
        return;
    }
    if (game_over_)
    {
        // Pointer and touch input reach the pop-up buttons through the UI layer.
        if (event.type == SDL_EVENT_KEY_DOWN) handle_game_over_key(manager, event.key.key);
        return;
    }
    if (event.type == SDL_EVENT_WINDOW_FOCUS_LOST || event.type == SDL_EVENT_DID_ENTER_BACKGROUND)
        swipe_active_ = false;
    if (event.type == SDL_EVENT_FINGER_DOWN || event.type == SDL_EVENT_FINGER_MOTION ||
        event.type == SDL_EVENT_FINGER_UP || event.type == SDL_EVENT_FINGER_CANCELED)
    {
        handle_touch(event);
        return;
    }
    if (event.type != SDL_EVENT_KEY_DOWN) return;

    const SDL_Keycode key = event.key.key;
    if (key == SDLK_ESCAPE)
        stop_game();
    else if (key == SDLK_UP || key == SDLK_W)
        change_direction(UP);
    else if (key == SDLK_DOWN || key == SDLK_S)
        change_direction(DOWN);
    else if (key == SDLK_LEFT || key == SDLK_A)
        change_direction(LEFT);
    else if (key == SDLK_RIGHT || key == SDLK_D)
        change_direction(RIGHT);
}

void UserGameView::start_game()
{
    playing_ = true;
    game_over_ = false;
    game_over_selection_ = GAME_OVER_NEW_GAME;
    swipe_active_ = false;
    game_.reset();
    timer_.reset();
}

void UserGameView::stop_game()
{
    playing_ = false;
    game_over_ = false;
    swipe_active_ = false;
    open(MenuPage::Play, "Manual game stopped.");
}

void UserGameView::handle_game_over_key(ViewManager &manager, SDL_Keycode key)
{
    if (key == SDLK_LEFT || key == SDLK_A || key == SDLK_UP || key == SDLK_W)
        game_over_selection_ = GAME_OVER_NEW_GAME;
    else if (key == SDLK_RIGHT || key == SDLK_D || key == SDLK_DOWN || key == SDLK_S)
        game_over_selection_ = GAME_OVER_MAIN_MENU;
    else if (key == SDLK_RETURN || key == SDLK_KP_ENTER || key == SDLK_SPACE)
        apply_game_over_action(manager, game_over_selection_ == GAME_OVER_NEW_GAME
                                            ? GameOverAction::NewGame
                                            : GameOverAction::MainMenu);
    else if (key == SDLK_ESCAPE)
        apply_game_over_action(manager, GameOverAction::MainMenu);
}

void UserGameView::apply_game_over_action(ViewManager &manager, GameOverAction action)
{
    if (action == GameOverAction::NewGame)
        start_game();
    else if (action == GameOverAction::MainMenu)
    {
        playing_ = false;
        game_over_ = false;
        manager.open_page(MenuPage::Main);
    }
}

void UserGameView::change_direction(Direction direction)
{
    Snake &snake = game_.board.snake();
    if ((static_cast<int>(direction) + 2) % 4 != snake.dir) snake.dir = direction;
}

void UserGameView::handle_touch(const SDL_Event &event)
{
    const SDL_TouchFingerEvent &finger = event.tfinger;
    if (finger.touchID == SDL_MOUSE_TOUCHID) return;
    if (event.type == SDL_EVENT_FINGER_DOWN)
    {
        if (swipe_active_) return;
        swipe_width_ = WINDOW_WIDTH;
        swipe_height_ = WINDOW_HEIGHT;
        if (finger.windowID != 0)
        {
            SDL_Window *window = SDL_GetWindowFromID(finger.windowID);
            if (window == nullptr || !SDL_GetWindowSize(window, &swipe_width_, &swipe_height_))
            {
                std::cerr << "Swipe coordinate error: " << SDL_GetError() << std::endl;
                return;
            }
        }
        swipe_active_ = true;
        swipe_handled_ = false;
        swipe_touch_ = finger.touchID;
        swipe_finger_ = finger.fingerID;
        swipe_x_ = finger.x;
        swipe_y_ = finger.y;
        return;
    }
    if (!swipe_active_ || finger.touchID != swipe_touch_ || finger.fingerID != swipe_finger_)
        return;
    if (event.type == SDL_EVENT_FINGER_CANCELED)
    {
        swipe_active_ = false;
        return;
    }
    if (!swipe_handled_)
    {
        const float dx = (finger.x - swipe_x_) * swipe_width_;
        const float dy = (finger.y - swipe_y_) * swipe_height_;
        const float threshold = 0.04f * std::min(swipe_width_, swipe_height_);
        if (std::max(std::abs(dx), std::abs(dy)) >= threshold)
        {
            change_direction(std::abs(dx) > std::abs(dy) ? (dx > 0 ? RIGHT : LEFT)
                                                         : (dy > 0 ? DOWN : UP));
            swipe_handled_ = true;
        }
    }
    if (event.type == SDL_EVENT_FINGER_UP) swipe_active_ = false;
}

void UserGameView::update(ViewManager &manager, Uint64 now_ms)
{
    (void)manager;
    if (!playing_ || game_over_ || !timer_.due(now_ms, MOVE_INTERVAL_MS)) return;

    game_.step(STRAIGHT);
    if (game_.game_over)
    {
        std::cout << "Game Over! Score: " << game_.score << std::endl;
        record_score(session_.best_manual_scores, game_.score);
        game_over_ = true;
        game_over_selection_ = GAME_OVER_NEW_GAME;
        swipe_active_ = false;
    }
}

void UserGameView::render(SDL_Renderer *renderer) const
{
    if (!playing_)
    {
        MenuView::render(renderer);
        return;
    }
    hud_action_ = draw_game(renderer, game_, session_.muted);
    if (game_over_) game_over_action_ = draw_game_over(game_, game_over_selection_);
}

void UserGameView::apply_ui_action(ViewManager &manager)
{
    const GameHudAction action = std::exchange(hud_action_, GameHudAction::None);
    const GameOverAction game_over_action = std::exchange(game_over_action_, GameOverAction::None);
    if (!playing_)
    {
        MenuView::apply_ui_action(manager);
        return;
    }
    if (action == GameHudAction::ToggleMute)
        session_.muted = !session_.muted;
    else if (action == GameHudAction::Exit)
        stop_game();
    else if (game_over_)
        apply_game_over_action(manager, game_over_action);
}

bool UserGameView::playing() const
{
    return playing_;
}

bool UserGameView::game_over() const
{
    return game_over_;
}

int UserGameView::game_over_selection() const
{
    return game_over_selection_;
}

SnakeGame &UserGameView::game()
{
    return game_;
}

void UserGameView::perform(MenuAction action, ViewManager &manager)
{
    (void)manager;
    if (action != MenuAction::StartManualGame) return;
    start_game();
}
