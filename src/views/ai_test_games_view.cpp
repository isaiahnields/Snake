#include "ai_test_games_view.h"

AITestGamesView::AITestGamesView() : MenuView(MenuPage::Test) {}

bool AITestGamesView::owns(MenuPage candidate) const
{
    return candidate == MenuPage::Test;
}

std::string AITestGamesView::title() const
{
    return "TEST ACTIVE AI";
}

std::vector<std::string> AITestGamesView::details() const
{
    return {"Visual AI testing is not implemented.", "Batch benchmarks are not implemented."};
}

std::vector<MenuItem> AITestGamesView::items() const
{
    return {menu_unavailable("Run Visual Game", page),
            menu_unavailable("Run Batch Benchmark (100 headless games)", page),
            menu_link("Return to AI Control Center", MenuPage::AI)};
}
