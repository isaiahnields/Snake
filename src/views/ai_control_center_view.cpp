#include "ai_control_center_view.h"

#include "../view_manager.h"

AIControlCenterView::AIControlCenterView(SessionState &session)
    : MenuView(MenuPage::AI), session_(session)
{
}

bool AIControlCenterView::owns(MenuPage candidate) const
{
    return candidate == MenuPage::AI || candidate == MenuPage::Profiles ||
           candidate == MenuPage::CreateProfile || candidate == MenuPage::Algorithms ||
           candidate == MenuPage::Train || candidate == MenuPage::Test ||
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
    case MenuPage::Train:
        return "TRAIN ACTIVE AI";
    case MenuPage::Test:
        return "TEST ACTIVE AI";
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
        return {"Only the built-in DQN agent is currently available.",
                "Algorithm selection requires the profile system."};
    case MenuPage::Train:
        return {"Training uses the active in-memory DQN agent.",
                "Escape stops training; learned weights remain this session."};
    case MenuPage::Test:
        return {"Visual testing runs one game with the session DQN.",
                "Batch benchmarks are not implemented."};
    default:
        return {"Active AI: session DQN (in memory only).",
                "Named profiles, saving and deletion are not implemented."};
    }
}

std::vector<MenuItem> AIControlCenterView::items() const
{
    const auto center = menu_link("Return to AI Control Center", MenuPage::AI);
    switch (page)
    {
    case MenuPage::Profiles:
        return {menu_unavailable("[x] Session DQN (Active)", page), center};
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
    case MenuPage::Train:
        return {menu_command("Episodes to Run: " + std::to_string(session_.training_episodes), page,
                             MenuAction::CycleTrainingEpisodes),
                menu_command(session_.visual_training
                                 ? "Render Mode: Visual (Slow to watch)"
                                 : "Render Mode: Headless (Fast, no game rendering)",
                             page, MenuAction::ToggleTrainingRenderMode),
                menu_unavailable("Start Training Session", page), center};
    case MenuPage::Test:
        return {menu_unavailable("Run Visual Game", page),
                menu_unavailable("Run Batch Benchmark (100 headless games)", page), center};
    case MenuPage::ManageProfiles:
        return {menu_unavailable("Delete Active AI Profile", page), center};
    default:
        return {menu_link("Select Active AI Profile", MenuPage::Profiles),
                menu_link("Create New AI Profile", MenuPage::CreateProfile),
                menu_link("Train Active AI", MenuPage::Train),
                menu_link("Train Active AI in Cloud", MenuPage::CloudTraining),
                menu_link("Test Active AI", MenuPage::Test),
                menu_link("Delete/Manage AI Profiles", MenuPage::ManageProfiles),
                menu_link("Return to Main Menu", MenuPage::Main)};
    }
}

void AIControlCenterView::perform(MenuAction action, ViewManager &manager)
{
    switch (action)
    {
    case MenuAction::CycleTrainingEpisodes:
        session_.training_episodes = session_.training_episodes == 1000  ? 100
                                     : session_.training_episodes == 100 ? 10
                                                                         : 1000;
        notice.clear();
        break;
    case MenuAction::ToggleTrainingRenderMode:
        session_.visual_training = !session_.visual_training;
        notice.clear();
        break;
    case MenuAction::StartTraining:
        manager.open(ViewId::TrainAI);
        break;
    case MenuAction::RunVisualTest:
        manager.open(ViewId::TestAI);
        break;
    default:
        break;
    }
}
