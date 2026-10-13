#pragma once

#include <string>
#include <vector>

enum class MenuPage
{
    Main,
    Play,
    Difficulty,
    HallOfFame,
    ManualGames,
    AIGames,
    AI,
    Profiles,
    CreateProfile,
    Algorithms,
    Train,
    Test,
    ManageProfiles,
    Vault,
    Recordings,
    Filters,
    PlayerFilter,
    Sort,
    ClearRecordings,
    Settings,
    GameSettings,
    RecordingSettings,
    AudioVisual,
    Exit
};

enum class MenuAction
{
    None,
    Manual,
    Training,
    AIPlay,
    Quit
};

struct MenuItem
{
    std::string label;
    MenuPage target;
    MenuAction action = MenuAction::None;
    bool available = true;
};

class Menu
{
  public:
    MenuPage page = MenuPage::Main;
    int selection = 0;
    int training_episodes = 1000;
    bool visual_training = true;
    std::string notice;

    std::string title() const;
    std::vector<std::string> details() const;
    std::vector<MenuItem> items() const;
    void open(MenuPage destination);
    void move(int offset);
    void back();
    MenuAction activate();
    MenuAction shortcut(int number);
};
