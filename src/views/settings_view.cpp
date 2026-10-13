#include "settings_view.h"

SettingsView::SettingsView() : MenuView(MenuPage::Settings) {}

bool SettingsView::owns(MenuPage candidate) const
{
    return candidate == MenuPage::Settings || candidate == MenuPage::GameSettings ||
           candidate == MenuPage::RecordingSettings || candidate == MenuPage::AudioVisual;
}

std::string SettingsView::title() const
{
    switch (page)
    {
    case MenuPage::GameSettings:
        return "GAME SETTINGS";
    case MenuPage::RecordingSettings:
        return "RECORDING SETTINGS";
    case MenuPage::AudioVisual:
        return "AUDIO & VISUAL SETTINGS";
    default:
        return "SETTINGS";
    }
}

std::vector<std::string> SettingsView::details() const
{
    switch (page)
    {
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
    default:
        return {};
    }
}

std::vector<MenuItem> SettingsView::items() const
{
    const auto settings = menu_link("Return to Settings", MenuPage::Settings);
    switch (page)
    {
    case MenuPage::GameSettings:
        return {menu_unavailable("Grid Size: 10x10 | 20x20 | 40x40", page),
                menu_unavailable("Wall Collision: Lethal | Wrap-around", page),
                menu_unavailable("Apple Spawn Rate: Single | Multi", page), settings};
    case MenuPage::RecordingSettings:
        return {menu_unavailable("Auto-Record Manual Games: ON", page),
                menu_unavailable("Auto-Record AI Training Games: OFF", page),
                menu_unavailable("Auto-Record AI Test Games: ON", page),
                menu_unavailable("Minimum Score Threshold: 50", page), settings};
    case MenuPage::AudioVisual:
        return {menu_unavailable("Master Volume: 0-100%", page),
                menu_unavailable("Theme: Classic Retro | Dark Mode | Neon Arcade", page), settings};
    default:
        return {menu_link("Game Settings", MenuPage::GameSettings),
                menu_link("Recording Settings", MenuPage::RecordingSettings),
                menu_link("Audio & Visual Settings", MenuPage::AudioVisual),
                menu_link("Return to Main Menu", MenuPage::Main)};
    }
}
