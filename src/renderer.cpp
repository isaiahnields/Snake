#include "renderer.h"

#include <cstring>

namespace
{
constexpr float MENU_TEXT_SCALE = 1.25f;

void draw_centered_menu_label(SDL_Renderer *renderer, const char *text, const SDL_FRect &bounds)
{
    const float text_width = static_cast<float>(std::strlen(text)) *
                             SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * MENU_TEXT_SCALE;
    const float text_height = SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * MENU_TEXT_SCALE;
    const float x = bounds.x + (bounds.w - text_width) / 2.0f;
    const float y = bounds.y + (bounds.h - text_height) / 2.0f;
    SDL_RenderDebugText(renderer, x / MENU_TEXT_SCALE, y / MENU_TEXT_SCALE, text);
}
} // namespace

int menu_item_at(float x, float y, const Menu &menu)
{
    if (x < MENU_LEFT || x >= MENU_LEFT + MENU_ROW_WIDTH || y < MENU_TOP) return -1;
    const int row = static_cast<int>((y - MENU_TOP) / MENU_ROW_HEIGHT);
    if (row >= static_cast<int>(menu.items().size()) ||
        y - MENU_TOP - row * MENU_ROW_HEIGHT >= MENU_ROW_HEIGHT - 6)
        return -1;
    return row;
}

void draw_menu(SDL_Renderer *renderer, const Menu &menu)
{
    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
    SDL_RenderClear(renderer);

    const SDL_FRect title_rect = {MENU_LEFT, 40.0f, MENU_ROW_WIDTH, 60.0f};
    SDL_SetRenderDrawColor(renderer, 100, 200, 255, 255);
    SDL_RenderFillRect(renderer, &title_rect);

    const auto entries = menu.items();
    for (size_t index = 0; index < entries.size(); ++index)
    {
        const SDL_FRect bounds = {MENU_LEFT, MENU_TOP + index * MENU_ROW_HEIGHT, MENU_ROW_WIDTH,
                                  MENU_ROW_HEIGHT - 6};
        const bool selected = static_cast<int>(index) == menu.selection;
        SDL_SetRenderDrawColor(renderer, selected ? 50 : 45, selected ? 120 : 45,
                               selected ? 85 : 45, 255);
        SDL_RenderFillRect(renderer, &bounds);
    }
    SDL_SetRenderScale(renderer, MENU_TEXT_SCALE, MENU_TEXT_SCALE);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    draw_centered_menu_label(renderer, "SNAKE", title_rect);
    const SDL_FRect subtitle = {MENU_LEFT, 110.0f, MENU_ROW_WIDTH, 40.0f};
    draw_centered_menu_label(renderer, menu.title().c_str(), subtitle);
    const auto details = menu.details();
    for (size_t index = 0; index < details.size(); ++index)
    {
        SDL_RenderDebugText(renderer, MENU_LEFT / MENU_TEXT_SCALE,
                            (170.0f + index * 24.0f) / MENU_TEXT_SCALE, details[index].c_str());
    }
    for (size_t index = 0; index < entries.size(); ++index)
    {
        SDL_SetRenderDrawColor(renderer, entries[index].available ? 255 : 170,
                               entries[index].available ? 255 : 170,
                               entries[index].available ? 255 : 170, 255);
        const int number = menu.page == MenuPage::Main && index == entries.size() - 1
                               ? 0
                               : static_cast<int>(index) + 1;
        const std::string text = std::to_string(number) + ". " + entries[index].label;
        SDL_RenderDebugText(renderer, (MENU_LEFT + 12) / MENU_TEXT_SCALE,
                            (MENU_TOP + index * MENU_ROW_HEIGHT + 16) / MENU_TEXT_SCALE,
                            text.c_str());
    }
    SDL_SetRenderDrawColor(renderer, 255, 210, 100, 255);
    SDL_RenderDebugText(renderer, MENU_LEFT / MENU_TEXT_SCALE, 770 / MENU_TEXT_SCALE,
                        menu.notice.c_str());
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
    SDL_RenderDebugText(renderer, MENU_LEFT / MENU_TEXT_SCALE, 820 / MENU_TEXT_SCALE,
                        "Up/Down: Select   Enter: Open   Number: Open   Esc: Back");
    SDL_RenderDebugText(renderer, MENU_LEFT / MENU_TEXT_SCALE, 848 / MENU_TEXT_SCALE,
                        "Click or tap an option to open it.");
    SDL_SetRenderScale(renderer, 1.0f, 1.0f);

    SDL_RenderPresent(renderer);
}

void draw_game(SDL_Renderer *renderer, SnakeGame &game)
{
    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 255, 50, 50, 255);
    SDL_FRect food_rect = {(float)(game.food.x * CELL_SIZE), (float)(game.food.y * CELL_SIZE),
                           (float)CELL_SIZE, (float)CELL_SIZE};
    SDL_RenderFillRect(renderer, &food_rect);

    for (size_t i = 0; i < game.snake.size(); ++i)
    {
        if (i == 0)
        {
            SDL_SetRenderDrawColor(renderer, 50, 255, 50, 255);
        }
        else
        {
            SDL_SetRenderDrawColor(renderer, 30, 150, 30, 255);
        }

        SDL_FRect snake_rect = {(float)(game.snake[i].x * CELL_SIZE + 1),
                                (float)(game.snake[i].y * CELL_SIZE + 1), (float)(CELL_SIZE - 2),
                                (float)(CELL_SIZE - 2)};
        SDL_RenderFillRect(renderer, &snake_rect);
    }

    SDL_RenderPresent(renderer);
}
