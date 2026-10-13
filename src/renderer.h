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

// Drawing helpers used by views. They only issue draw instructions; the ViewManager clears
// and presents each frame.
void draw_menu(SDL_Renderer *renderer, const MenuView &menu);
int menu_item_at(float x, float y, const MenuView &menu);
void draw_game(SDL_Renderer *renderer, const SnakeGame &game);
void draw_status(SDL_Renderer *renderer, const std::string &text);
void draw_info_screen(SDL_Renderer *renderer, const std::string &title,
                      const std::vector<std::string> &lines, const std::string &hint);
