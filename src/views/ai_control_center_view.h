#pragma once

#include "menu_view.h"

// Sets up and manages AI players, and launches local, cloud and test sessions for them.
class AIControlCenterView : public MenuView
{
  public:
    AIControlCenterView();

    bool owns(MenuPage candidate) const override;
    std::string title() const override;
    std::vector<std::string> details() const override;
    std::vector<MenuItem> items() const override;

};
