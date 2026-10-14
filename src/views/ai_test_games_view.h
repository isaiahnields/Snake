#pragma once

#include "menu_view.h"

class AITestGamesView : public MenuView
{
  public:
    AITestGamesView();

    bool owns(MenuPage candidate) const override;
    std::string title() const override;
    std::vector<std::string> details() const override;
    std::vector<MenuItem> items() const override;
};
