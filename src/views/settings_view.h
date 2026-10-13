#pragma once

#include "menu_view.h"

class SettingsView : public MenuView
{
  public:
    SettingsView();

    bool owns(MenuPage candidate) const override;
    std::string title() const override;
    std::vector<std::string> details() const override;
    std::vector<MenuItem> items() const override;
};
