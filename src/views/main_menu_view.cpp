#include "main_menu_view.h"

MainMenuView::MainMenuView() : MenuView(MenuPage::Main) {}

bool MainMenuView::owns(MenuPage candidate) const
{
    return candidate == MenuPage::Main;
}

std::string MainMenuView::title() const
{
    return "MAIN MENU";
}

std::vector<MenuItem> MainMenuView::items() const
{
    return {menu_link("Play Game (Manual Mode)", MenuPage::Play),
            menu_link("AI Control Center (Create, Select, Train, Test)", MenuPage::AI),
            menu_link("Hall of Fame (Best Games & Metrics)", MenuPage::HallOfFame),
            menu_link("Replay Vault (Browse Recordings)", MenuPage::Vault),
            menu_link("Settings", MenuPage::Settings),
            menu_link("Exit Game", MenuPage::Exit)};
}

int MainMenuView::shortcut_number(std::size_t index) const
{
    return index + 1 == items().size() ? 0 : MenuView::shortcut_number(index);
}
