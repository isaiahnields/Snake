#pragma once

#include "../view.h"

class SignUpOrSignInView : public View
{
  public:
    void handle_event(ViewManager &manager, const SDL_Event &event) override;
    void render(SDL_Renderer *renderer) const override;
    void apply_ui_action(ViewManager &manager) override;
    void on_enter(ViewManager &manager) override;

  private:
    int selection_ = 0;
    mutable int clicked_ = -1;
};
