#pragma once

#include "menu_view.h"

// Lets the user browse and replay previously recorded games.
class ReplayVaultMenu : public MenuView
{
  public:
    ReplayVaultMenu();

    bool owns(MenuPage candidate) const override;
    std::string title() const override;
    std::vector<std::string> details() const override;
    std::vector<MenuItem> items() const override;
};
