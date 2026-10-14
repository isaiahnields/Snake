#pragma once

#include "menu_view.h"

class AITestCloudGamesView : public MenuView
{
  public:
    AITestCloudGamesView();

    bool owns(MenuPage candidate) const override;
    std::string title() const override;
    std::vector<std::string> details() const override;
    std::vector<MenuItem> items() const override;
};
