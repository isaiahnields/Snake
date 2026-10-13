#include "replay_vault_menu.h"

ReplayVaultMenu::ReplayVaultMenu() : MenuView(MenuPage::Vault) {}

bool ReplayVaultMenu::owns(MenuPage candidate) const
{
    return candidate == MenuPage::Vault || candidate == MenuPage::Recordings ||
           candidate == MenuPage::Filters || candidate == MenuPage::PlayerFilter ||
           candidate == MenuPage::Sort || candidate == MenuPage::ClearRecordings;
}

std::string ReplayVaultMenu::title() const
{
    switch (page)
    {
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
    default:
        return "REPLAY VAULT (BROWSE RECORDINGS)";
    }
}

std::vector<std::string> ReplayVaultMenu::details() const
{
    switch (page)
    {
    case MenuPage::Filters:
    case MenuPage::PlayerFilter:
    case MenuPage::Sort:
        return {"Recording search, filtering and sorting are not implemented."};
    default:
        return {"No recordings. Recording storage is not implemented.",
                "Planned recording actions: Play, View Metrics, Delete.",
                "Pagination will be available when recordings exist."};
    }
}

std::vector<MenuItem> ReplayVaultMenu::items() const
{
    const auto vault = menu_link("Return to Replay Vault", MenuPage::Vault);
    const auto filters = menu_link("Return to Search & Filter", MenuPage::Filters);
    switch (page)
    {
    case MenuPage::Recordings:
        return {vault};
    case MenuPage::Filters:
        return {menu_link("Filter by Player", MenuPage::PlayerFilter),
                menu_unavailable("Minimum Score: 100", page), menu_link("Sort by", MenuPage::Sort),
                vault};
    case MenuPage::PlayerFilter:
        return {menu_unavailable("Any", page), menu_unavailable("Manual", page),
                menu_unavailable("Specific AI Model", page), filters};
    case MenuPage::Sort:
        return {menu_unavailable("Date (Desc)", page), menu_unavailable("Score (Desc)", page),
                menu_unavailable("Duration", page), filters};
    case MenuPage::ClearRecordings:
        return {menu_unavailable("Confirm Clear All Recordings", page),
                menu_link("Cancel", MenuPage::Vault)};
    default:
        return {menu_link("Browse All Recordings", MenuPage::Recordings),
                menu_link("Search & Filter Recordings", MenuPage::Filters),
                menu_link("Clear All Recordings", MenuPage::ClearRecordings),
                menu_link("Return to Main Menu", MenuPage::Main)};
    }
}
