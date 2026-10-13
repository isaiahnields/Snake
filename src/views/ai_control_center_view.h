#pragma once

#include "../session_state.h"
#include "menu_view.h"

// Sets up and manages AI players, and launches local, cloud and test sessions for them.
class AIControlCenterView : public MenuView
{
  public:
    explicit AIControlCenterView(SessionState &session);

    bool owns(MenuPage candidate) const override;
    std::string title() const override;
    std::vector<std::string> details() const override;
    std::vector<MenuItem> items() const override;

  protected:
    void perform(MenuAction action, ViewManager &manager) override;

  private:
    SessionState &session_;
};
