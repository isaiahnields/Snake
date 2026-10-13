#pragma once

#include "../session_state.h"
#include "menu_view.h"

// Shows the best scores achieved by manual players and AI players this session.
class HallOfFameView : public MenuView
{
  public:
    explicit HallOfFameView(const SessionState &session);

    bool owns(MenuPage candidate) const override;
    std::string title() const override;
    std::vector<std::string> details() const override;
    std::vector<MenuItem> items() const override;

  private:
    const SessionState &session_;
};
