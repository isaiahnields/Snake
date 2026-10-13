#pragma once

#include "../view.h"

#include <cstddef>
#include <string>
#include <vector>

// Every menu page, each owned by exactly one MenuView.
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
    CloudTraining,
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
    StartManualGame,
    CycleTrainingEpisodes,
    ToggleTrainingRenderMode,
    Quit
};

// Activating an item runs its action, or opens target when it has no action. The last item
// of every page links to its parent, which is also where Escape goes.
struct MenuItem
{
    std::string label;
    MenuPage target;
    MenuAction action = MenuAction::None;
    bool available = true;
};

MenuItem menu_link(const std::string &label, MenuPage target);
MenuItem menu_command(const std::string &label, MenuPage page, MenuAction action);
MenuItem menu_unavailable(const std::string &label, MenuPage page);

// Base for views that present one or more pages of selectable menu items.
class MenuView : public View
{
  public:
    MenuPage page;
    int selection = 0;
    std::string notice;

    explicit MenuView(MenuPage initial_page);

    virtual bool owns(MenuPage candidate) const = 0;
    virtual std::string title() const = 0;
    virtual std::vector<std::string> details() const;
    virtual std::vector<MenuItem> items() const = 0;
    // Number key that activates the item at index (0 may be used for the last item).
    virtual int shortcut_number(std::size_t index) const;

    void open(MenuPage destination, const std::string &message = {});
    void move(int offset);
    void back(ViewManager &manager);
    void activate(ViewManager &manager);
    void shortcut(int number, ViewManager &manager);
    void apply_ui_action(ViewManager &manager);

    void handle_event(ViewManager &manager, const SDL_Event &event) override;
    void render(SDL_Renderer *renderer) const override;
    MenuView *as_menu() override;

  protected:
    virtual void perform(MenuAction action, ViewManager &manager);

  private:
    mutable int clicked_item_ = -1;
    void navigate(MenuPage target, ViewManager &manager);
};
