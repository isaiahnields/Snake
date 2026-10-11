#include "renderer.h"

void draw_menu(SDL_Renderer* renderer, GameMode selected_mode) {
    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    SDL_FRect title_rect = {50, 50, 300, 60};
    SDL_SetRenderDrawColor(renderer, 100, 200, 255, 255);
    SDL_RenderFillRect(renderer, &title_rect);

    SDL_FRect option1_rect = {50, 150, 300, 50};
    SDL_SetRenderDrawColor(renderer, 100, selected_mode == MANUAL ? 255 : 150, 100, 255);
    SDL_RenderFillRect(renderer, &option1_rect);

    SDL_FRect option2_rect = {50, 250, 300, 50};
    SDL_SetRenderDrawColor(renderer, 100, selected_mode == TRAINING ? 255 : 150, 100, 255);
    SDL_RenderFillRect(renderer, &option2_rect);

    SDL_FRect option3_rect = {50, 350, 300, 50};
    SDL_SetRenderDrawColor(renderer, 100, selected_mode == AI_PLAY ? 255 : 150, 100, 255);
    SDL_RenderFillRect(renderer, &option3_rect);

    SDL_SetRenderScale(renderer, 2.0f, 2.0f);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDebugText(renderer, 80.0f, 36.0f, "SNAKE");
    SDL_RenderDebugText(renderer, 76.0f, 84.0f, "Manual");
    SDL_RenderDebugText(renderer, 68.0f, 134.0f, "Training");
    SDL_RenderDebugText(renderer, 72.0f, 184.0f, "AI Play");
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