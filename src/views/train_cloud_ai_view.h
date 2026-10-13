#pragma once

#include "menu_view.h"

// Entry point for training the active AI on remote cloud infrastructure.
class TrainCloudAIView : public MenuView
{
  public:
    TrainCloudAIView();

    bool owns(MenuPage candidate) const override;
    std::string title() const override;
    std::vector<std::string> details() const override;
    std::vector<MenuItem> items() const override;
};
