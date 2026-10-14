#include "renderer.h"

#include <imgui.h>

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
    ImGui::TextWrapped("Up/Down: Select   Enter: Open   Number: Open   Esc: Back");
    ImGui::TextWrapped("Click or tap an option to open it.");
    ImGui::PopStyleVar();
    ImGui::PopID();
    ImGui::End();
    return clicked;
}

void draw_game(SDL_Renderer *renderer, const SnakeGame &game)
{
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
