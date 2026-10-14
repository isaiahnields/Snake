#pragma once

#include "menu_view.h"

class MainMenuView : public MenuView
{
  public:
    MainMenuView();

    bool owns(MenuPage candidate) const override;
    std::string title() const override;
    std::vector<MenuItem> items() const override;
    int shortcut_number(std::size_t index) const override;

  protected:
    void perform(MenuAction action, ViewManager &manager) override;
};
