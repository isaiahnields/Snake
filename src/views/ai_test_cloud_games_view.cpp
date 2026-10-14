#include "ai_test_cloud_games_view.h"

AITestCloudGamesView::AITestCloudGamesView() : MenuView(MenuPage::CloudTesting) {}

bool AITestCloudGamesView::owns(MenuPage candidate) const
{
    return candidate == MenuPage::CloudTesting;
}

std::string AITestCloudGamesView::title() const
{
    return "TEST ACTIVE AI IN CLOUD";
}

std::vector<std::string> AITestCloudGamesView::details() const
{
    return {"Cloud testing runs AI games on a remote testing service.",
            "No cloud service is configured; cloud testing is not implemented.",
            "Use Test Active AI to test locally on this device."};
}

std::vector<MenuItem> AITestCloudGamesView::items() const
{
    return {menu_unavailable("Cloud Provider: <none configured>", page),
            menu_unavailable("Start Cloud Testing Session", page),
            menu_link("Return to AI Control Center", MenuPage::AI)};
}
