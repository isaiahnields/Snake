#pragma once

#include "menu_view.h"

// Final confirmation before saving and quitting the application.
class SaveAndExitView : public MenuView
{
  public:
    SaveAndExitView();

    bool owns(MenuPage candidate) const override;
    std::string title() const override;
    std::vector<std::string> details() const override;
    std::vector<MenuItem> items() const override;

  protected:
    void perform(MenuAction action, ViewManager &manager) override;
};
