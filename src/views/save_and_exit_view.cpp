#include "save_and_exit_view.h"

#include "../view_manager.h"

SaveAndExitView::SaveAndExitView() : MenuView(MenuPage::Exit) {}

bool SaveAndExitView::owns(MenuPage candidate) const
{
    return candidate == MenuPage::Exit;
}

std::string SaveAndExitView::title() const
{
    return "EXIT GAME";
}

std::vector<std::string> SaveAndExitView::details() const
{
    return {"There is no persistent data to save yet.",
            "Quitting discards the session's scores and preferences."};
}

std::vector<MenuItem> SaveAndExitView::items() const
{
    return {menu_command("Save & Quit", page, MenuAction::Quit),
            menu_link("Cancel", MenuPage::Main)};
}

void SaveAndExitView::perform(MenuAction action, ViewManager &manager)
{
    if (action == MenuAction::Quit) manager.request_quit();
}
