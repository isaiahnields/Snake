#pragma once

#include "menu.h"
#include "snake_game.h"

#include <SDL3/SDL.h>

constexpr float MENU_LEFT = 40.0f;
constexpr float MENU_TOP = 290.0f;
constexpr float MENU_ROW_HEIGHT = 52.0f;
constexpr float MENU_ROW_WIDTH = WINDOW_WIDTH - 2 * MENU_LEFT;

void draw_menu(SDL_Renderer *renderer, const Menu &menu);
int menu_item_at(float x, float y, const Menu &menu);
void draw_game(SDL_Renderer *renderer, SnakeGame &game);
