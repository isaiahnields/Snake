#include "ai_control_center_view.h"

#include "../view_manager.h"

AIControlCenterView::AIControlCenterView() : MenuView(MenuPage::AI) {}

bool AIControlCenterView::owns(MenuPage candidate) const
{
    return candidate == MenuPage::AI || candidate == MenuPage::Profiles ||
           candidate == MenuPage::CreateProfile || candidate == MenuPage::Algorithms ||
           candidate == MenuPage::ManageProfiles;
}

std::string AIControlCenterView::title() const
{
    switch (page)
    {
    case MenuPage::Profiles:
        return "SELECT ACTIVE AI PROFILE";
    case MenuPage::CreateProfile:
        return "CREATE NEW AI PROFILE";
    case MenuPage::Algorithms:
        return "CHOOSE CORE ALGORITHM";
    case MenuPage::ManageProfiles:
        return "DELETE / MANAGE AI PROFILES";
    default:
        return "AI CONTROL CENTER";
    }
}

std::vector<std::string> AIControlCenterView::details() const
{
    switch (page)
    {
    case MenuPage::CreateProfile:
        return {"Profile creation and editable fields are not implemented.",
                "Shown values are proposed defaults, not active settings."};
    case MenuPage::Algorithms:
        return {"AI algorithms are not implemented.",
                "Algorithm selection requires the profile system."};
    default:
        return {"No active AI is available.",
                "Named profiles, saving and deletion are not implemented."};
    }
}

std::vector<MenuItem> AIControlCenterView::items() const
{
    const auto center = menu_link("Return to AI Control Center", MenuPage::AI);
    switch (page)
    {
    case MenuPage::Profiles:
        return {menu_unavailable("Select Active AI Profile", page), center};
    case MenuPage::CreateProfile:
        return {menu_unavailable("Enter AI Name: <unnamed>", page),
                menu_link("Choose Core Algorithm", MenuPage::Algorithms),
                menu_unavailable("Learning Rate (Alpha): 0.01", page),
                menu_unavailable("Discount Factor (Gamma): 0.95", page),
                menu_unavailable("Exploration Rate (Epsilon): 0.10", page),
                menu_unavailable("Save and Set as Active AI", page),
                center};
    case MenuPage::Algorithms:
        return {menu_unavailable("Q-Learning (Tabular Reinforcement Learning)", page),
                menu_unavailable("Deep Q-Network (Neural Network RL)", page),
                menu_unavailable("NEAT (Genetic Algorithm / Evolution)", page),
                menu_unavailable("A* Search (Heuristic Pathfinding)", page),
                menu_link("Return to Create Profile", MenuPage::CreateProfile)};
    case MenuPage::ManageProfiles:
        return {menu_unavailable("Delete Active AI Profile", page), center};
    default:
        return {menu_link("Select Active AI Profile", MenuPage::Profiles),
                menu_link("Create New AI Profile", MenuPage::CreateProfile),
                menu_link("Train Active AI", MenuPage::Train),
                menu_link("Test Active AI", MenuPage::Test),
                menu_link("Train Active AI in Cloud", MenuPage::CloudTraining),
                menu_link("Test Active AI in Cloud", MenuPage::CloudTesting),
                menu_link("Delete/Manage AI Profiles", MenuPage::ManageProfiles),
                menu_link("Return to Main Menu", MenuPage::Main)};
    }
}
