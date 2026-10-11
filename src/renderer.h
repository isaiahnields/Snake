#pragma once

#include "snake_game.h"

#include <SDL3/SDL.h>

void draw_menu(SDL_Renderer* renderer, GameMode selected_mode);
void draw_game(SDL_Renderer* renderer, SnakeGame& game);