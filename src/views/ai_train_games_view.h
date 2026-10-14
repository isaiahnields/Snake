#pragma once

#include "../session_state.h"
#include "menu_view.h"

class AITrainGamesView : public MenuView
{
  public:
    explicit AITrainGamesView(SessionState &session);

    bool owns(MenuPage candidate) const override;
    std::string title() const override;
    std::vector<std::string> details() const override;
    std::vector<MenuItem> items() const override;

  protected:
    void perform(MenuAction action, ViewManager &manager) override;

  private:
    SessionState &session_;
};
