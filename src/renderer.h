#pragma once

#include "snake_game.h"
#include "views/menu_view.h"

#include <SDL3/SDL.h>
#include <string>
#include <vector>

constexpr float MENU_LEFT = 40.0f;
constexpr float MENU_TOP = 290.0f;
constexpr float MENU_ROW_HEIGHT = 52.0f;
constexpr float MENU_ROW_WIDTH = WINDOW_WIDTH - 2 * MENU_LEFT;

// Placement of the in-game HUD and board, in ImGui display (window) coordinates.
struct GameLayout
{
    SDL_FRect hud;
    SDL_FRect mute_button;
    SDL_FRect exit_button;
    SDL_FRect board;
    float cell_size;
    float border;
    // Game Over pop-up, centered on the board, with its two side-by-side buttons.
    SDL_FRect game_over_popup;
    SDL_FRect new_game_button;
    SDL_FRect main_menu_button;
};

enum class GameHudAction
{
    None,
    ToggleMute,
    Exit
};

enum class GameOverAction
{
    None,
    NewGame,
    MainMenu
};

// Index of each Game Over button for keyboard and controller selection.
constexpr int GAME_OVER_NEW_GAME = 0;
constexpr int GAME_OVER_MAIN_MENU = 1;

// Fits and centers the bordered board in the display space below the HUD.
GameLayout game_layout(float width, float height, int columns = GRID_WIDTH, int rows = GRID_HEIGHT);

// UI helpers require an active ImGui frame. The manager applies menu clicks after rendering.
void begin_form_screen(const char *title);
bool draw_menu_button(const std::string &text, bool selected, float height, bool available = true);
int draw_menu(SDL_Renderer *renderer, const MenuView &menu);
GameHudAction draw_game(SDL_Renderer *renderer, const SnakeGame &game, bool muted);
// Draws the Game Over pop-up over the game drawn by draw_game; selection is a GAME_OVER_* index.
GameOverAction draw_game_over(const SnakeGame &game, int selection);
void draw_status(SDL_Renderer *renderer, const std::string &text);
void draw_info_screen(SDL_Renderer *renderer, const std::string &title,
                      const std::vector<std::string> &lines, const std::string &hint);
