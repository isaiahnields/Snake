#include "test_helpers.h"

#include "../src/menu.h"

#include <set>

int main()
{
    Menu menu;
    CHECK(menu.title() == "MAIN MENU");
    CHECK(menu.items().size() == 6);
    menu.move(-1);
    CHECK(menu.selection == 5);
    menu.move(1);
    CHECK(menu.selection == 0);
    CHECK(menu.shortcut(1) == MenuAction::None);
    CHECK(menu.page == MenuPage::Play);
    CHECK(menu.shortcut(1) == MenuAction::Manual);
    CHECK(menu.shortcut(2) == MenuAction::None);
    CHECK(menu.page == MenuPage::Difficulty);
    CHECK(menu.shortcut(1) == MenuAction::None);
    CHECK(!menu.notice.empty());
    menu.back();
    CHECK(menu.page == MenuPage::Play);
    CHECK(menu.notice.empty());
    menu.back();
    CHECK(menu.page == MenuPage::Main);

    menu.shortcut(3);
    CHECK(menu.page == MenuPage::AI);
    menu.shortcut(3);
    CHECK(menu.page == MenuPage::Train);
    menu.shortcut(1);
    CHECK(menu.training_episodes == 100);
    menu.shortcut(1);
    CHECK(menu.training_episodes == 10);
    menu.shortcut(1);
    CHECK(menu.training_episodes == 1000);
    menu.shortcut(2);
    CHECK(!menu.visual_training);
    menu.shortcut(2);
    CHECK(menu.visual_training);
    CHECK(menu.shortcut(3) == MenuAction::Training);
    menu.back();
    menu.shortcut(4);
    CHECK(menu.page == MenuPage::Test);
    CHECK(menu.shortcut(1) == MenuAction::AIPlay);
    CHECK(menu.shortcut(2) == MenuAction::None);
    CHECK(!menu.notice.empty());

    menu.open(MenuPage::Main);
    CHECK(menu.shortcut(0) == MenuAction::None);
    CHECK(menu.page == MenuPage::Exit);
    menu.shortcut(2);
    CHECK(menu.page == MenuPage::Main);
    menu.back();
    CHECK(menu.page == MenuPage::Exit);
    CHECK(menu.shortcut(1) == MenuAction::Quit);
    menu.back();
    CHECK(menu.page == MenuPage::Main);
    CHECK(menu.shortcut(9) == MenuAction::None);
    CHECK(menu.page == MenuPage::Main);

    std::set<MenuPage> visited;
    std::vector<MenuPage> pending = {MenuPage::Main};
    while (!pending.empty())
    {
        const auto page = pending.back();
        pending.pop_back();
        if (!visited.insert(page).second) continue;
        menu.open(page);
        CHECK(!menu.title().empty());
        const auto entries = menu.items();
        CHECK(!entries.empty());
        for (size_t index = 0; index < entries.size(); ++index)
        {
            CHECK(!entries[index].label.empty());
            menu.open(page);
            menu.selection = static_cast<int>(index);
            const auto action = menu.activate();
            if (!entries[index].available)
            {
                CHECK(action == MenuAction::None);
                CHECK(menu.page == page);
                CHECK(!menu.notice.empty());
            }
            else if (entries[index].action == MenuAction::None)
            {
                pending.push_back(menu.page);
            }
            else
            {
                CHECK(action == entries[index].action);
            }
        }
        menu.open(page);
        menu.move(-1);
        CHECK(menu.selection == static_cast<int>(entries.size()) - 1);
        menu.move(1);
        CHECK(menu.selection == 0);
        menu.back();
        if (page != MenuPage::Main && page != MenuPage::Exit)
            CHECK(menu.page == entries.back().target);
    }
    CHECK(visited.size() == static_cast<size_t>(MenuPage::Exit) + 1);
    return TEST_RESULT();
}
