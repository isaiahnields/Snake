#include "renderer.h"

#include <cstring>

namespace {
constexpr float MENU_WIDTH = 300.0f;
constexpr float TITLE_HEIGHT = 60.0f;
constexpr float OPTION_HEIGHT = 50.0f;
constexpr float MENU_TEXT_SCALE = 2.0f;
constexpr float MENU_HEIGHT = 350.0f;

void draw_centered_menu_label(SDL_Renderer* renderer, const char* text, const SDL_FRect& bounds) {
    const float text_width = static_cast<float>(std::strlen(text)) *
                             SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * MENU_TEXT_SCALE;
    const float text_height = SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * MENU_TEXT_SCALE;
    const float x = bounds.x + (bounds.w - text_width) / 2.0f;
    const float y = bounds.y + (bounds.h - text_height) / 2.0f;
    SDL_RenderDebugText(renderer, x / MENU_TEXT_SCALE, y / MENU_TEXT_SCALE, text);
}
}

void draw_menu(SDL_Renderer* renderer, GameMode selected_mode) {
    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
    SDL_RenderClear(renderer);

    const float menu_x = (WINDOW_WIDTH - MENU_WIDTH) / 2.0f;
    const float menu_y = (WINDOW_HEIGHT - MENU_HEIGHT) / 2.0f;
    const float options_y = menu_y + TITLE_HEIGHT + 40.0f;

    SDL_FRect title_rect = {menu_x, menu_y, MENU_WIDTH, TITLE_HEIGHT};
    SDL_SetRenderDrawColor(renderer, 100, 200, 255, 255);
    SDL_RenderFillRect(renderer, &title_rect);

    SDL_FRect option1_rect = {menu_x, options_y, MENU_WIDTH, OPTION_HEIGHT};
    SDL_SetRenderDrawColor(renderer, 100, selected_mode == MANUAL ? 255 : 150, 100, 255);
    SDL_RenderFillRect(renderer, &option1_rect);

    SDL_FRect option2_rect = {menu_x, options_y + 100.0f, MENU_WIDTH, OPTION_HEIGHT};
    SDL_SetRenderDrawColor(renderer, 100, selected_mode == TRAINING ? 255 : 150, 100, 255);
    SDL_RenderFillRect(renderer, &option2_rect);

    SDL_FRect option3_rect = {menu_x, options_y + 200.0f, MENU_WIDTH, OPTION_HEIGHT};
    SDL_SetRenderDrawColor(renderer, 100, selected_mode == AI_PLAY ? 255 : 150, 100, 255);
    SDL_RenderFillRect(renderer, &option3_rect);

    SDL_SetRenderScale(renderer, MENU_TEXT_SCALE, MENU_TEXT_SCALE);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    draw_centered_menu_label(renderer, "SNAKE", title_rect);
    draw_centered_menu_label(renderer, "Manual", option1_rect);
    draw_centered_menu_label(renderer, "Training", option2_rect);
    draw_centered_menu_label(renderer, "AI Play", option3_rect);
    SDL_SetRenderScale(renderer, 1.0f, 1.0f);

    SDL_RenderPresent(renderer);
}

void draw_game(SDL_Renderer* renderer, SnakeGame& game) {
    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 255, 50, 50, 255);
    SDL_FRect food_rect = {
        (float)(game.food.x * CELL_SIZE),
        (float)(game.food.y * CELL_SIZE),
        (float)CELL_SIZE,
        (float)CELL_SIZE
    };
    SDL_RenderFillRect(renderer, &food_rect);

    for (size_t i = 0; i < game.snake.size(); ++i) {
        if (i == 0) {
            SDL_SetRenderDrawColor(renderer, 50, 255, 50, 255);
        } else {
            SDL_SetRenderDrawColor(renderer, 30, 150, 30, 255);
        }

        SDL_FRect snake_rect = {
            (float)(game.snake[i].x * CELL_SIZE + 1),
            (float)(game.snake[i].y * CELL_SIZE + 1),
            (float)(CELL_SIZE - 2),
            (float)(CELL_SIZE - 2)
        };
        SDL_RenderFillRect(renderer, &snake_rect);
    }

    SDL_RenderPresent(renderer);
}