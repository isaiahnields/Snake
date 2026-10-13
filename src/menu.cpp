#include "menu.h"

namespace
{
MenuItem link(const std::string &label, MenuPage target)
{
    return {label, target};
}

MenuItem unavailable(const std::string &label, MenuPage page)
{
    return {label + " [Not implemented]", page, MenuAction::None, false};
}
} // namespace

std::string Menu::title() const
{
    switch (page)
    {
    case MenuPage::Main:
        return "MAIN MENU";
    case MenuPage::Play:
        return "PLAY GAME (MANUAL MODE)";
    case MenuPage::Difficulty:
        return "CHOOSE DIFFICULTY";
    case MenuPage::HallOfFame:
        return "HALL OF FAME (BEST GAMES & METRICS)";
    case MenuPage::ManualGames:
        return "TOP MANUAL GAMES";
    case MenuPage::AIGames:
        return "TOP AI GAMES";
    case MenuPage::AI:
        return "AI CONTROL CENTER";
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
    case MenuPage::Vault:
        return "REPLAY VAULT (BROWSE RECORDINGS)";
    case MenuPage::Recordings:
        return "BROWSE ALL RECORDINGS";
    case MenuPage::Filters:
        return "SEARCH & FILTER RECORDINGS";
    case MenuPage::PlayerFilter:
        return "FILTER BY PLAYER";
    case MenuPage::Sort:
        return "SORT RECORDINGS";
    case MenuPage::ClearRecordings:
        return "CLEAR ALL RECORDINGS";
    case MenuPage::Settings:
        return "SETTINGS";
    case MenuPage::GameSettings:
        return "GAME SETTINGS";
    case MenuPage::RecordingSettings:
        return "RECORDING SETTINGS";
    case MenuPage::AudioVisual:
        return "AUDIO & VISUAL SETTINGS";
    case MenuPage::Exit:
        return "EXIT GAME";
    }
    return "";
}

std::vector<std::string> Menu::details() const
{
    switch (page)
    {
    case MenuPage::Play:
    case MenuPage::Difficulty:
        return {"Current game: 30x30, lethal walls, one apple, 100ms moves.",
                "Difficulty presets and obstacles are not implemented."};
    case MenuPage::ManualGames:
        return {"No saved manual games. Leaderboards are not implemented.",
                "Planned metrics: time survived, total moves, moves/apple.",
                "Watch Replay requires the recording system."};
    case MenuPage::AIGames:
        return {"No saved AI games. Leaderboards are not implemented.",
                "Planned metrics: grid coverage, generation, compute time.",
                "Watch Replay requires the recording system."};
    case MenuPage::AI:
    case MenuPage::Profiles:
    case MenuPage::ManageProfiles:
        return {"Active AI: session DQN (in memory only).",
                "Named profiles, saving and deletion are not implemented."};
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
    case MenuPage::Recordings:
    case MenuPage::Vault:
    case MenuPage::ClearRecordings:
        return {"No recordings. Recording storage is not implemented.",
                "Planned recording actions: Play, View Metrics, Delete.",
                "Pagination will be available when recordings exist."};
    case MenuPage::Filters:
    case MenuPage::PlayerFilter:
    case MenuPage::Sort:
        return {"Recording search, filtering and sorting are not implemented."};
    case MenuPage::GameSettings:
        return {"Applies to manual play and AI environments.",
                "Current: 30x30, lethal walls, single apple.",
                "Configurable environments are not implemented."};
    case MenuPage::RecordingSettings:
        return {"Recording is not implemented; no games are auto-recorded.",
                "Warning: training recordings can fill disk space.",
                "Shown values are proposed defaults, not active settings."};
    case MenuPage::AudioVisual:
        return {"Audio, volume control and themes are not implemented."};
    case MenuPage::Exit:
        return {"There is no persistent data to save yet.",
                "Quitting discards the session's learned DQN weights."};
    default:
        return {};
    }
}

std::vector<MenuItem> Menu::items() const
{
    const auto main = link("Return to Main Menu", MenuPage::Main);
    switch (page)
    {
    case MenuPage::Main:
        return {link("Play Game (Manual Mode)", MenuPage::Play),
                link("Hall of Fame (Best Games & Metrics)", MenuPage::HallOfFame),
                link("AI Control Center (Create, Select, Train, Test)", MenuPage::AI),
                link("Replay Vault (Browse Recordings)", MenuPage::Vault),
                link("Settings", MenuPage::Settings),
                link("Exit Game", MenuPage::Exit)};
    case MenuPage::Play:
        return {{"Start New Game", page, MenuAction::Manual},
                link("Choose Difficulty", MenuPage::Difficulty),
                main};
    case MenuPage::Difficulty:
        return {unavailable("Easy (Slow Speed, No Obstacles)", page),
                unavailable("Normal (Medium Speed, Standard Board)", page),
                unavailable("Hard (Fast Speed, Dynamic Obstacles)", page),
                link("Return to Play Game", MenuPage::Play)};
    case MenuPage::HallOfFame:
        return {link("Top Manual Games", MenuPage::ManualGames),
                link("Top AI Games", MenuPage::AIGames), main};
    case MenuPage::ManualGames:
    case MenuPage::AIGames:
        return {link("Return to Hall of Fame", MenuPage::HallOfFame)};
    case MenuPage::AI:
        return {link("Select Active AI Profile", MenuPage::Profiles),
                link("Create New AI Profile", MenuPage::CreateProfile),
                link("Train Active AI", MenuPage::Train),
                link("Test Active AI", MenuPage::Test),
                link("Delete/Manage AI Profiles", MenuPage::ManageProfiles),
                main};
    case MenuPage::Profiles:
        return {unavailable("[x] Session DQN (Active)", page),
                link("Return to AI Control Center", MenuPage::AI)};
    case MenuPage::CreateProfile:
        return {unavailable("Enter AI Name: <unnamed>", page),
                link("Choose Core Algorithm", MenuPage::Algorithms),
                unavailable("Learning Rate (Alpha): 0.01", page),
                unavailable("Discount Factor (Gamma): 0.95", page),
                unavailable("Exploration Rate (Epsilon): 0.10", page),
                unavailable("Save and Set as Active AI", page),
                link("Return to AI Control Center", MenuPage::AI)};
    case MenuPage::Algorithms:
        return {unavailable("Q-Learning (Tabular Reinforcement Learning)", page),
                unavailable("Deep Q-Network (Neural Network RL)", page),
                unavailable("NEAT (Genetic Algorithm / Evolution)", page),
                unavailable("A* Search (Heuristic Pathfinding)", page),
                link("Return to Create Profile", MenuPage::CreateProfile)};
    case MenuPage::Train:
        return {link("Episodes to Run: " + std::to_string(training_episodes), page),
                link(visual_training ? "Render Mode: Visual (Slow to watch)"
                                     : "Render Mode: Headless (Fast, no game rendering)",
                     page),
                {"Start Training Session", page, MenuAction::Training},
                link("Return to AI Control Center", MenuPage::AI)};
    case MenuPage::Test:
        return {{"Run Visual Game", page, MenuAction::AIPlay},
                unavailable("Run Batch Benchmark (100 headless games)", page),
                link("Return to AI Control Center", MenuPage::AI)};
    case MenuPage::ManageProfiles:
        return {unavailable("Delete Active AI Profile", page),
                link("Return to AI Control Center", MenuPage::AI)};
    case MenuPage::Vault:
        return {link("Browse All Recordings", MenuPage::Recordings),
                link("Search & Filter Recordings", MenuPage::Filters),
                link("Clear All Recordings", MenuPage::ClearRecordings), main};
    case MenuPage::Recordings:
        return {link("Return to Replay Vault", MenuPage::Vault)};
    case MenuPage::Filters:
        return {link("Filter by Player", MenuPage::PlayerFilter),
                unavailable("Minimum Score: 100", page), link("Sort by", MenuPage::Sort),
                link("Return to Replay Vault", MenuPage::Vault)};
    case MenuPage::PlayerFilter:
        return {unavailable("Any", page), unavailable("Manual", page),
                unavailable("Specific AI Model", page),
                link("Return to Search & Filter", MenuPage::Filters)};
    case MenuPage::Sort:
        return {unavailable("Date (Desc)", page), unavailable("Score (Desc)", page),
                unavailable("Duration", page),
                link("Return to Search & Filter", MenuPage::Filters)};
    case MenuPage::ClearRecordings:
        return {unavailable("Confirm Clear All Recordings", page), link("Cancel", MenuPage::Vault)};
    case MenuPage::Settings:
        return {link("Game Settings", MenuPage::GameSettings),
                link("Recording Settings", MenuPage::RecordingSettings),
                link("Audio & Visual Settings", MenuPage::AudioVisual), main};
    case MenuPage::GameSettings:
        return {unavailable("Grid Size: 10x10 | 20x20 | 40x40", page),
                unavailable("Wall Collision: Lethal | Wrap-around", page),
                unavailable("Apple Spawn Rate: Single | Multi", page),
                link("Return to Settings", MenuPage::Settings)};
    case MenuPage::RecordingSettings:
        return {unavailable("Auto-Record Manual Games: ON", page),
                unavailable("Auto-Record AI Training Games: OFF", page),
                unavailable("Auto-Record AI Test Games: ON", page),
                unavailable("Minimum Score Threshold: 50", page),
                link("Return to Settings", MenuPage::Settings)};
    case MenuPage::AudioVisual:
        return {unavailable("Master Volume: 0-100%", page),
                unavailable("Theme: Classic Retro | Dark Mode | Neon Arcade", page),
                link("Return to Settings", MenuPage::Settings)};
    case MenuPage::Exit:
        return {{"Save & Quit", page, MenuAction::Quit}, link("Cancel", MenuPage::Main)};
    }
    return {};
}

void Menu::open(MenuPage destination)
{
    page = destination;
    selection = 0;
    notice.clear();
}

void Menu::move(int offset)
{
    const int count = static_cast<int>(items().size());
    selection = ((selection + offset) % count + count) % count;
}

void Menu::back()
{
    if (page == MenuPage::Main)
        open(MenuPage::Exit);
    else if (page == MenuPage::Exit)
        open(MenuPage::Main);
    else
        open(items().back().target);
}

MenuAction Menu::activate()
{
    const auto entries = items();
    const auto &item = entries.at(selection);
    if (!item.available)
    {
        notice = "Not implemented yet. No changes were made.";
        return MenuAction::None;
    }
    if (page == MenuPage::Train && selection < 2)
    {
        if (selection == 0)
            training_episodes = training_episodes == 1000  ? 100
                                : training_episodes == 100 ? 10
                                                           : 1000;
        else
            visual_training = !visual_training;
        notice.clear();
        return MenuAction::None;
    }
    if (item.action != MenuAction::None) return item.action;
    open(item.target);
    return MenuAction::None;
}

MenuAction Menu::shortcut(int number)
{
    const int count = static_cast<int>(items().size());
    const int index = page == MenuPage::Main && number == 0 ? count - 1 : number - 1;
    if (index >= 0 && index < count)
    {
        selection = index;
        return activate();
    }
    return MenuAction::None;
}
