#pragma once

#include "snake_game.h"

#include <SDL3/SDL.h>

void draw_menu(SDL_Renderer* renderer, int selected_option);
void draw_game(SDL_Renderer* renderer, SnakeGame& game);