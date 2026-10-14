#include "sign_up_or_sign_in_view.h"

#include "../renderer.h"
#include "../view_manager.h"

#include <imgui.h>
#include <utility>

void SignUpOrSignInView::on_enter(ViewManager &manager)
{
    (void)manager;
    clicked_ = -1;
}

void SignUpOrSignInView::handle_event(ViewManager &manager, const SDL_Event &event)
{
    if (event.type != SDL_EVENT_KEY_DOWN) return;
    const auto key = event.key.key;
    if (key == SDLK_UP || key == SDLK_DOWN) selection_ = 1 - selection_;
    if (key == SDLK_1 || key == SDLK_2 || key == SDLK_RETURN || key == SDLK_SPACE)
    {
        if (key == SDLK_1) selection_ = 0;
        if (key == SDLK_2) selection_ = 1;
        manager.open(selection_ == 0 ? ViewId::SignUp : ViewId::SignIn);
    }
    if (key == SDLK_ESCAPE) manager.request_quit();
}

void SignUpOrSignInView::render(SDL_Renderer *renderer) const
{
    (void)renderer;
    begin_form_screen("PLAYER ACCOUNT");
    ImGui::TextWrapped("Sign up or sign in to play Snake.");
    if (ImGui::Button(selection_ == 0 ? "> 1. Sign Up" : "1. Sign Up",
                      ImVec2(ImGui::GetContentRegionAvail().x, 48)))
        clicked_ = 0;
    if (ImGui::Button(selection_ == 1 ? "> 2. Sign In" : "2. Sign In",
                      ImVec2(ImGui::GetContentRegionAvail().x, 48)))
        clicked_ = 1;
    ImGui::TextWrapped("Up/Down: Select   Enter: Open   Esc: Exit");
    ImGui::End();
}

void SignUpOrSignInView::apply_ui_action(ViewManager &manager)
{
    const int clicked = std::exchange(clicked_, -1);
    if (clicked >= 0) manager.open(clicked == 0 ? ViewId::SignUp : ViewId::SignIn);
}
