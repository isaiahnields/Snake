#include "renderer.h"

#include <algorithm>
#include <cmath>
#include <imgui.h>
#include <iostream>

namespace
{
float ui_scale()
{
    const ImVec2 size = ImGui::GetIO().DisplaySize;
    return size.x < size.y ? size.x / WINDOW_WIDTH : size.y / WINDOW_HEIGHT;
}

void begin_screen(const std::string &subtitle)
{
    const float scale = ui_scale();
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(MENU_LEFT * scale, 40 * scale));
    ImGui::Begin("Snake UI", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus);
    ImGui::PopStyleVar();
    ImGui::SetWindowFontScale(1.25f * scale);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(100 / 255.0f, 200 / 255.0f, 1, 1));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 1));
    const ImVec2 banner_size(ImGui::GetContentRegionAvail().x, 60 * scale);
    ImGui::BeginChild("Banner", banner_size, ImGuiChildFlags_None,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    const ImVec2 text_size = ImGui::CalcTextSize("SNAKE");
    ImGui::SetCursorPos(
        ImVec2((banner_size.x - text_size.x) / 2, (banner_size.y - text_size.y) / 2));
    ImGui::TextUnformatted("SNAKE");
    ImGui::EndChild();
    ImGui::PopStyleColor(2);
    ImGui::Spacing();
    ImGui::TextWrapped("%s", subtitle.c_str());
    ImGui::Spacing();
}
} // namespace

void begin_form_screen(const char *title)
{
    begin_screen(title);
}

bool draw_menu_button(const std::string &text, bool selected, float height, bool available)
{
    selected = selected && (SDL_HasKeyboard() || SDL_HasGamepad());
    ImGui::PushStyleColor(ImGuiCol_Button, selected
                                               ? ImVec4(50 / 255.0f, 120 / 255.0f, 85 / 255.0f, 1)
                                               : ImVec4(45 / 255.0f, 45 / 255.0f, 45 / 255.0f, 1));
    ImGui::PushStyleColor(ImGuiCol_Text,
                          available ? ImVec4(1, 1, 1, 1) : ImVec4(.67f, .67f, .67f, 1));
    const bool clicked =
        ImGui::Button(text.c_str(), ImVec2(ImGui::GetContentRegionAvail().x, height));
    ImGui::PopStyleColor(2);
    return clicked;
}

int draw_menu(SDL_Renderer *renderer, const MenuView &menu)
{
    (void)renderer;
    begin_screen(menu.title());
    ImGui::PushID(static_cast<int>(menu.page));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8 * ui_scale(), 3 * ui_scale()));
    const auto details = menu.details();
    for (const auto &line : details)
        ImGui::TextWrapped("%s", line.c_str());
    const float scale = ui_scale();
    if (ImGui::GetCursorPosY() < MENU_TOP * scale) ImGui::SetCursorPosY(MENU_TOP * scale);
    int clicked = -1;
    const auto entries = menu.items();
    for (size_t index = 0; index < entries.size(); ++index)
    {
        ImGui::PushID(static_cast<int>(index));
        const bool selected = static_cast<int>(index) == menu.selection;
        const std::string text =
            std::to_string(menu.shortcut_number(index)) + ". " + entries[index].label;
        if (draw_menu_button(text, selected, (MENU_ROW_HEIGHT - 6) * scale,
                             entries[index].available))
            clicked = static_cast<int>(index);
        ImGui::PopID();
        ImGui::Dummy(ImVec2(0, 2 * scale));
    }
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 210 / 255.0f, 100 / 255.0f, 1));
    ImGui::TextWrapped("%s", menu.notice.c_str());
    ImGui::PopStyleColor();
    ImGui::TextWrapped("W/S or Up/Down: Select   Enter: Open   Number: Open   Esc: Back");
    ImGui::TextWrapped("Click or tap an option to open it.");
    ImGui::TextWrapped("Controller: D-pad/left stick: Select   A/Cross: Open   B/Circle: Back");
    ImGui::PopStyleVar();
    ImGui::PopID();
    ImGui::End();
    return clicked;
}

GameLayout game_layout(float width, float height, int columns, int rows)
{
    GameLayout layout{};
    const float unit = std::min(width, height) / WINDOW_WIDTH;
    const float hud_height = std::max(36.0f, std::round(60.0f * unit));
    const float margin = std::max(8.0f, std::round(16.0f * unit));
    layout.border = std::max(2.0f, std::round(4.0f * unit));

    layout.hud = {0, 0, width, hud_height};
    const float padding = std::round(hud_height * 0.15f);
    const float button_height = hud_height - 2 * padding;
    const float exit_width = std::round(1.6f * hud_height);
    const float mute_width = std::round(2.2f * hud_height);
    layout.exit_button = {width - margin - exit_width, padding, exit_width, button_height};
    layout.mute_button = {layout.exit_button.x - padding - mute_width, padding, mute_width,
                          button_height};

    const float board_area_top = layout.hud.y + layout.hud.h;
    const float board_area_height = height - board_area_top;
    const float available_width = width - 2 * (margin + layout.border);
    const float available_height = board_area_height - 2 * (margin + layout.border);
    float cell = std::min(available_width / columns, available_height / rows);
    // Whole-unit cells keep the grid lines crisp.
    cell = cell >= 1 ? std::floor(cell) : std::max(cell, 0.0f);
    layout.cell_size = cell;
    const float board_width = cell * columns;
    const float board_height = cell * rows;
    layout.board = {std::round((width - board_width) / 2),
                    std::round(board_area_top + (board_area_height - board_height) / 2),
                    board_width, board_height};

    const float popup_width = std::min(std::round(480.0f * unit), width - 2 * margin);
    const float popup_height = std::round(300.0f * unit);
    layout.game_over_popup = {std::round(layout.board.x + (layout.board.w - popup_width) / 2),
                              std::round(layout.board.y + (layout.board.h - popup_height) / 2),
                              popup_width, popup_height};
    const float popup_padding = std::round(24.0f * unit);
    const float popup_button_height = std::round(56.0f * unit);
    const float popup_button_width = std::round((popup_width - 3 * popup_padding) / 2);
    const float popup_button_y =
        layout.game_over_popup.y + popup_height - popup_padding - popup_button_height;
    layout.new_game_button = {layout.game_over_popup.x + popup_padding, popup_button_y,
                              popup_button_width, popup_button_height};
    layout.main_menu_button = {layout.game_over_popup.x + popup_width - popup_padding -
                                   popup_button_width,
                               popup_button_y, popup_button_width, popup_button_height};
    return layout;
}

namespace
{
void draw_board(SDL_Renderer *renderer, const Board &board, const GameLayout &layout,
                ImVec2 framebuffer_scale)
{
    int logical_width = 0, logical_height = 0;
    SDL_RendererLogicalPresentation presentation = SDL_LOGICAL_PRESENTATION_DISABLED;
    float scale_x = 1.0f, scale_y = 1.0f;
    if (!SDL_GetRenderLogicalPresentation(renderer, &logical_width, &logical_height,
                                          &presentation) ||
        !SDL_GetRenderScale(renderer, &scale_x, &scale_y))
    {
        std::cerr << "Board render state error: " << SDL_GetError() << std::endl;
        return;
    }
    // The layout uses window coordinates, so bypass any logical presentation while drawing.
    SDL_SetRenderLogicalPresentation(renderer, 0, 0, SDL_LOGICAL_PRESENTATION_DISABLED);
    SDL_SetRenderScale(renderer, framebuffer_scale.x, framebuffer_scale.y);

    const SDL_FRect &area = layout.board;
    const float border = layout.border;
    const SDL_FRect frame = {area.x - border, area.y - border, area.w + 2 * border,
                             area.h + 2 * border};
    SDL_SetRenderDrawColor(renderer, 220, 220, 220, 255);
    SDL_RenderFillRect(renderer, &frame);
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
    SDL_RenderFillRect(renderer, &area);

    const float size = layout.cell_size;
    const float gap = size >= 6 ? 1.0f : 0.0f;
    const auto cell_rect = [&](Point p, float inset)
    {
        return SDL_FRect{area.x + p.x * size + inset, area.y + p.y * size + inset, size - 2 * inset,
                         size - 2 * inset};
    };
    // Each cell leaves a gap on its top and left edges to show the grid.
    const auto grid_rect = [&](Point p)
    {
        return SDL_FRect{area.x + p.x * size + gap, area.y + p.y * size + gap, size - gap,
                         size - gap};
    };

    std::vector<SDL_FRect> cells;
    cells.reserve(board.cells().size());
    for (const Cell &cell : board.cells())
        cells.push_back(grid_rect(cell.position()));
    SDL_SetRenderDrawColor(renderer, 18, 18, 18, 255);
    SDL_RenderFillRects(renderer, cells.data(), static_cast<int>(cells.size()));

    const SDL_FRect food = grid_rect(board.food().position());
    SDL_SetRenderDrawColor(renderer, 255, 50, 50, 255);
    SDL_RenderFillRect(renderer, &food);

    const float snake_inset = size >= 6 ? std::max(1.0f, std::round(size * 0.05f)) : 0.0f;
    const auto &body = board.snake().body;
    for (size_t i = 0; i < body.size(); ++i)
    {
        if (i == 0)
            SDL_SetRenderDrawColor(renderer, 50, 255, 50, 255);
        else
            SDL_SetRenderDrawColor(renderer, 30, 150, 30, 255);
        const SDL_FRect segment = cell_rect(body[i], snake_inset);
        SDL_RenderFillRect(renderer, &segment);
    }

    SDL_SetRenderScale(renderer, scale_x, scale_y);
    SDL_SetRenderLogicalPresentation(renderer, logical_width, logical_height, presentation);
}

bool draw_hud_button(const char *label, const SDL_FRect &rect)
{
    ImGui::SetCursorScreenPos(ImVec2(rect.x, rect.y));
    return ImGui::Button(label, ImVec2(rect.w, rect.h));
}

GameHudAction draw_game_hud(const GameLayout &layout, int score, bool muted)
{
    const SDL_FRect &hud = layout.hud;
    ImGui::SetNextWindowPos(ImVec2(hud.x, hud.y));
    ImGui::SetNextWindowSize(ImVec2(hud.w, hud.h));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(20 / 255.0f, 20 / 255.0f, 20 / 255.0f, 1));
    ImGui::Begin("Game HUD", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollWithMouse |
                     ImGuiWindowFlags_NoBringToFrontOnFocus);
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
    ImGui::SetWindowFontScale(hud.h / 40.0f);

    const std::string score_text = "Score: " + std::to_string(score);
    const ImVec2 text_size = ImGui::CalcTextSize(score_text.c_str());
    const float margin = hud.x + hud.w - (layout.exit_button.x + layout.exit_button.w);
    ImGui::SetCursorScreenPos(ImVec2(hud.x + margin, hud.y + (hud.h - text_size.y) / 2));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 210 / 255.0f, 100 / 255.0f, 1));
    ImGui::TextUnformatted(score_text.c_str());
    ImGui::PopStyleColor();

    GameHudAction action = GameHudAction::None;
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(45 / 255.0f, 45 / 255.0f, 45 / 255.0f, 1));
    if (draw_hud_button(muted ? "Unmute###mute" : "Mute###mute", layout.mute_button))
        action = GameHudAction::ToggleMute;
    if (draw_hud_button("Exit###exit", layout.exit_button)) action = GameHudAction::Exit;
    ImGui::PopStyleColor();
    ImGui::End();
    return action;
}
} // namespace

GameHudAction draw_game(SDL_Renderer *renderer, const SnakeGame &game, bool muted)
{
    const ImGuiIO &io = ImGui::GetIO();
    const GameLayout layout =
        game_layout(io.DisplaySize.x, io.DisplaySize.y, game.board.width(), game.board.height());
    draw_board(renderer, game.board, layout, io.DisplayFramebufferScale);
    return draw_game_hud(layout, game.score, muted);
}

GameOverAction draw_game_over(const SnakeGame &game, int selection)
{
    const ImGuiIO &io = ImGui::GetIO();
    const GameLayout layout =
        game_layout(io.DisplaySize.x, io.DisplaySize.y, game.board.width(), game.board.height());
    const SDL_FRect &board = layout.board;
    // Dim the frozen board so the pop-up stands out.
    ImGui::GetBackgroundDrawList()->AddRectFilled(ImVec2(board.x, board.y),
                                                  ImVec2(board.x + board.w, board.y + board.h),
                                                  IM_COL32(0, 0, 0, 150));

    const SDL_FRect &popup = layout.game_over_popup;
    ImGui::SetNextWindowPos(ImVec2(popup.x, popup.y));
    ImGui::SetNextWindowSize(ImVec2(popup.w, popup.h));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, std::max(1.0f, layout.border / 2));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(20 / 255.0f, 20 / 255.0f, 20 / 255.0f, 1));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(220 / 255.0f, 220 / 255.0f, 220 / 255.0f, 1));
    ImGui::Begin("Game Over", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);
    const float font_scale = popup.h / 200.0f;

    const auto centered_text = [&](const std::string &text, float y, float scale, ImVec4 color)
    {
        ImGui::SetWindowFontScale(scale);
        const ImVec2 size = ImGui::CalcTextSize(text.c_str());
        ImGui::SetCursorScreenPos(ImVec2(popup.x + (popup.w - size.x) / 2, y));
        ImGui::PushStyleColor(ImGuiCol_Text, color);
        ImGui::TextUnformatted(text.c_str());
        ImGui::PopStyleColor();
        return size.y;
    };
    const float gap = popup.h * 0.08f;
    const float title_height = centered_text("GAME OVER", popup.y + gap, 1.6f * font_scale,
                                             ImVec4(1, 80 / 255.0f, 80 / 255.0f, 1));
    centered_text("Score: " + std::to_string(game.score), popup.y + 2 * gap + title_height,
                  font_scale, ImVec4(1, 210 / 255.0f, 100 / 255.0f, 1));

    ImGui::SetWindowFontScale(font_scale);
    // Highlight the keyboard/controller selection the same way menus do.
    const bool show_selection = SDL_HasKeyboard() || SDL_HasGamepad();
    const auto popup_button = [&](const char *label, const SDL_FRect &rect, bool selected)
    {
        ImGui::PushStyleColor(ImGuiCol_Button,
                              selected && show_selection
                                  ? ImVec4(50 / 255.0f, 120 / 255.0f, 85 / 255.0f, 1)
                                  : ImVec4(45 / 255.0f, 45 / 255.0f, 45 / 255.0f, 1));
        const bool clicked = draw_hud_button(label, rect);
        ImGui::PopStyleColor();
        return clicked;
    };
    GameOverAction action = GameOverAction::None;
    if (popup_button("New Game", layout.new_game_button, selection == GAME_OVER_NEW_GAME))
        action = GameOverAction::NewGame;
    if (popup_button("Main Menu", layout.main_menu_button, selection == GAME_OVER_MAIN_MENU))
        action = GameOverAction::MainMenu;
    ImGui::End();
    return action;
}
void draw_status(SDL_Renderer *renderer, const std::string &text)
{
    (void)renderer;
    ImGui::SetNextWindowPos(ImVec2(6, 6));
    ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x - 12, 90 * ui_scale()));
    ImGui::Begin("Game status", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings |
                     ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoBackground);
    ImGui::SetWindowFontScale(1.25f * ui_scale());
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 210 / 255.0f, 100 / 255.0f, 1));
    ImGui::TextWrapped("%s", text.c_str());
    ImGui::PopStyleColor();
    ImGui::End();
}

void draw_info_screen(SDL_Renderer *renderer, const std::string &title,
                      const std::vector<std::string> &lines, const std::string &hint)
{
    (void)renderer;
    begin_screen(title);
    for (const auto &line : lines)
        ImGui::TextWrapped("%s", line.c_str());
    ImGui::Spacing();
    ImGui::TextWrapped("%s", hint.c_str());
    ImGui::End();
}
