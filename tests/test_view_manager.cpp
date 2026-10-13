#include "test_helpers.h"

#include "../src/renderer.h"
#include "../src/view_manager.h"
#include "../src/views/manual_game_view.h"

#include <set>
#include <string>
#include <vector>

namespace
{
SDL_Event key_event(SDL_Keycode key, bool repeat = false)
{
    SDL_Event event{};
    event.type = SDL_EVENT_KEY_DOWN;
    event.key.key = key;
    event.key.repeat = repeat;
    return event;
}

void press(ViewManager &manager, SDL_Keycode key)
{
    manager.handle_event(key_event(key));
}

void click(ViewManager &manager, float x, float y, Uint8 button = SDL_BUTTON_LEFT)
{
    SDL_Event event{};
    event.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
    event.button.button = button;
    event.button.x = x;
    event.button.y = y;
    manager.handle_event(event);
}

bool showing(ViewManager &manager, ViewId id, MenuPage page)
{
    const MenuView *menu = manager.active().as_menu();
    return manager.active_id() == id && menu != nullptr && menu->page == page;
}

const std::string &notice(ViewManager &manager)
{
    static const std::string none = "<not a menu>";
    const MenuView *menu = manager.active().as_menu();
    return menu != nullptr ? menu->notice : none;
}

bool starts_with(const std::string &text, const std::string &prefix)
{
    return text.rfind(prefix, 0) == 0;
}

void test_main_menu_and_manual_game()
{
    ViewManager manager;
    SessionState &session = manager.session();
    CHECK(showing(manager, ViewId::MainMenu, MenuPage::Main));
    MenuView &main_menu = *manager.active().as_menu();
    CHECK(main_menu.title() == "MAIN MENU");
    CHECK(main_menu.items().size() == 6);
    press(manager, SDLK_UP);
    CHECK(main_menu.selection == 5);
    press(manager, SDLK_DOWN);
    CHECK(main_menu.selection == 0);

    press(manager, SDLK_1);
    CHECK(showing(manager, ViewId::ManualGame, MenuPage::Play));
    press(manager, SDLK_2);
    CHECK(showing(manager, ViewId::ManualGame, MenuPage::Difficulty));
    press(manager, SDLK_1);
    CHECK(showing(manager, ViewId::ManualGame, MenuPage::Difficulty));
    CHECK(!notice(manager).empty());
    press(manager, SDLK_ESCAPE);
    CHECK(showing(manager, ViewId::ManualGame, MenuPage::Play));
    CHECK(notice(manager).empty());
    press(manager, SDLK_ESCAPE);
    CHECK(showing(manager, ViewId::MainMenu, MenuPage::Main));

    press(manager, SDLK_1);
    press(manager, SDLK_1);
    auto &manual = static_cast<ManualGameView &>(manager.view(ViewId::ManualGame));
    CHECK(manager.active_id() == ViewId::ManualGame);
    CHECK(manual.playing());
    press(manager, SDLK_LEFT);
    CHECK(manual.game().dir == RIGHT);
    press(manager, SDLK_UP);
    CHECK(manual.game().dir == UP);

    Uint64 now_ms = 0;
    for (int update = 0; update < 100 && session.best_manual_scores.empty(); ++update)
    {
        manual.update(manager, now_ms);
        now_ms += ManualGameView::MOVE_INTERVAL_MS;
    }
    CHECK(session.best_manual_scores.size() == 1);
    CHECK(manual.playing());
    CHECK(!manual.game().game_over);

    press(manager, SDLK_ESCAPE);
    CHECK(!manual.playing());
    CHECK(showing(manager, ViewId::ManualGame, MenuPage::Play));
    CHECK(notice(manager) == "Manual game stopped.");

    manager.open_page(MenuPage::ManualGames);
    CHECK(showing(manager, ViewId::HallOfFame, MenuPage::ManualGames));
    CHECK(starts_with(manager.active().as_menu()->details().at(0), "Best manual scores"));
    manager.open_page(MenuPage::AIGames);
    CHECK(starts_with(manager.active().as_menu()->details().at(0), "No AI test games"));
}

void test_ai_views()
{
    ViewManager manager;
    SessionState &session = manager.session();
    manager.open_page(MenuPage::AI);
    CHECK(showing(manager, ViewId::AIControlCenter, MenuPage::AI));
    CHECK(manager.active().as_menu()->items().size() == 7);
    CHECK(manager.active().as_menu()->details().at(0) == "No active AI is available.");

    press(manager, SDLK_1);
    CHECK(showing(manager, ViewId::AIControlCenter, MenuPage::Profiles));
    CHECK(!manager.active().as_menu()->items().at(0).available);
    press(manager, SDLK_1);
    CHECK(notice(manager) == "Not implemented yet. No changes were made.");
    press(manager, SDLK_ESCAPE);

    press(manager, SDLK_3);
    CHECK(showing(manager, ViewId::AIControlCenter, MenuPage::Train));
    CHECK(manager.active().as_menu()->details().at(0) == "AI training is not implemented.");
    press(manager, SDLK_1);
    CHECK(session.training_episodes == 100);
    press(manager, SDLK_1);
    CHECK(session.training_episodes == 10);
    press(manager, SDLK_1);
    CHECK(session.training_episodes == 1000);
    press(manager, SDLK_1);
    press(manager, SDLK_1);
    press(manager, SDLK_2);
    CHECK(!session.visual_training);
    press(manager, SDLK_2);
    CHECK(session.visual_training);
    press(manager, SDLK_2);
    CHECK(!session.visual_training);

    press(manager, SDLK_3);
    CHECK(showing(manager, ViewId::AIControlCenter, MenuPage::Train));
    CHECK(notice(manager) == "Not implemented yet. No changes were made.");
    CHECK(session.training_episodes == 10);
    CHECK(!session.visual_training);
    manager.update();
    CHECK(showing(manager, ViewId::AIControlCenter, MenuPage::Train));
    CHECK(session.best_ai_scores.empty());

    press(manager, SDLK_ESCAPE);
    CHECK(showing(manager, ViewId::AIControlCenter, MenuPage::AI));

    press(manager, SDLK_4);
    CHECK(showing(manager, ViewId::TrainCloudAI, MenuPage::CloudTraining));
    press(manager, SDLK_2);
    CHECK(showing(manager, ViewId::TrainCloudAI, MenuPage::CloudTraining));
    CHECK(!notice(manager).empty());
    press(manager, SDLK_ESCAPE);
    CHECK(showing(manager, ViewId::AIControlCenter, MenuPage::AI));

    press(manager, SDLK_5);
    CHECK(showing(manager, ViewId::AIControlCenter, MenuPage::Test));
    CHECK(manager.active().as_menu()->details().at(0) == "Visual AI testing is not implemented.");
    press(manager, SDLK_1);
    CHECK(showing(manager, ViewId::AIControlCenter, MenuPage::Test));
    CHECK(notice(manager) == "Not implemented yet. No changes were made.");
    press(manager, SDLK_2);
    CHECK(showing(manager, ViewId::AIControlCenter, MenuPage::Test));
    CHECK(!notice(manager).empty());
    manager.update();
    CHECK(showing(manager, ViewId::AIControlCenter, MenuPage::Test));
    CHECK(session.best_ai_scores.empty());

    manager.open_page(MenuPage::AIGames);
    CHECK(starts_with(manager.active().as_menu()->details().at(0), "No AI test games"));
}

void test_exit_and_input_filtering()
{
    ViewManager manager;
    press(manager, SDLK_0);
    CHECK(showing(manager, ViewId::SaveAndExit, MenuPage::Exit));
    press(manager, SDLK_2);
    CHECK(showing(manager, ViewId::MainMenu, MenuPage::Main));
    press(manager, SDLK_ESCAPE);
    CHECK(showing(manager, ViewId::SaveAndExit, MenuPage::Exit));
    CHECK(manager.active().as_menu()->details().at(1) ==
          "Quitting discards the session's scores and preferences.");
    press(manager, SDLK_ESCAPE);
    CHECK(showing(manager, ViewId::MainMenu, MenuPage::Main));
    press(manager, SDLK_9);
    CHECK(showing(manager, ViewId::MainMenu, MenuPage::Main));

    manager.handle_event(key_event(SDLK_1, true));
    CHECK(showing(manager, ViewId::MainMenu, MenuPage::Main));

    click(manager, MENU_LEFT + 2, MENU_TOP + 2, SDL_BUTTON_RIGHT);
    CHECK(showing(manager, ViewId::MainMenu, MenuPage::Main));
    click(manager, MENU_LEFT - 1, MENU_TOP + 2);
    CHECK(showing(manager, ViewId::MainMenu, MenuPage::Main));
    // Pointer activation is handled by ImGui and covered by the renderer tests.
    click(manager, MENU_LEFT + 2, MENU_TOP + MENU_ROW_HEIGHT + 2);
    CHECK(showing(manager, ViewId::MainMenu, MenuPage::Main));

    SDL_Event close{};
    close.type = SDL_EVENT_QUIT;
    manager.handle_event(close);
    CHECK(showing(manager, ViewId::SaveAndExit, MenuPage::Exit));
    CHECK(!manager.quit_requested());
    press(manager, SDLK_1);
    CHECK(manager.quit_requested());
}

void test_every_page()
{
    ViewManager manager;
    const int page_count = static_cast<int>(MenuPage::Exit) + 1;
    for (int value = 0; value < page_count; ++value)
    {
        int owners = 0;
        for (int id = 0; id < static_cast<int>(ViewId::Count); ++id)
        {
            const MenuView *menu = manager.view(static_cast<ViewId>(id)).as_menu();
            if (menu != nullptr && menu->owns(static_cast<MenuPage>(value))) ++owners;
        }
        CHECK(owners == 1);
    }

    std::set<MenuPage> visited;
    std::vector<MenuPage> pending = {MenuPage::Main};
    while (!pending.empty())
    {
        const auto page = pending.back();
        pending.pop_back();
        if (!visited.insert(page).second) continue;
        manager.open_page(page);
        MenuView *menu = manager.active().as_menu();
        CHECK(menu != nullptr && menu->page == page);
        if (menu == nullptr) continue;
        const ViewId owner = manager.active_id();
        CHECK(!menu->title().empty());
        const auto entries = menu->items();
        CHECK(!entries.empty());
        for (std::size_t index = 0; index < entries.size(); ++index)
        {
            CHECK(!entries[index].label.empty());
            manager.open_page(page);
            menu->selection = static_cast<int>(index);
            menu->activate(manager);
            if (!entries[index].available)
            {
                CHECK(showing(manager, owner, page));
                CHECK(!menu->notice.empty());
            }
            else if (entries[index].action == MenuAction::None)
            {
                const MenuView *destination = manager.active().as_menu();
                CHECK(destination != nullptr && destination->page == entries[index].target);
                CHECK(notice(manager).empty());
                pending.push_back(entries[index].target);
            }
        }
        manager.open_page(page);
        menu->move(-1);
        CHECK(menu->selection == static_cast<int>(entries.size()) - 1);
        menu->move(1);
        CHECK(menu->selection == 0);
        press(manager, SDLK_ESCAPE);
        const MenuView *parent = manager.active().as_menu();
        CHECK(parent != nullptr);
        if (parent != nullptr)
        {
            int parent_owners = 0;
            for (int id = 0; id < static_cast<int>(ViewId::Count); ++id)
            {
                const MenuView *candidate = manager.view(static_cast<ViewId>(id)).as_menu();
                if (candidate != nullptr && candidate->owns(parent->page)) ++parent_owners;
            }
            CHECK(parent_owners == 1);
        }
    }
    CHECK(visited.size() == static_cast<std::size_t>(page_count));
}

void test_session_state()
{
    ViewManager first;
    ViewManager second;
    SessionState &session = first.session();
    const SessionState &other = second.session();
    CHECK(session.training_episodes == 1000);
    CHECK(session.visual_training);
    CHECK(session.best_manual_scores.empty());
    CHECK(session.best_ai_scores.empty());

    session.training_episodes = 10;
    session.visual_training = false;
    record_score(session.best_manual_scores, 7);
    record_score(session.best_ai_scores, 4);
    first.open_page(MenuPage::Train);
    first.open_page(MenuPage::Main);
    CHECK(session.training_episodes == 10);
    CHECK(!session.visual_training);
    CHECK((session.best_manual_scores == std::vector<int>{7}));
    CHECK((session.best_ai_scores == std::vector<int>{4}));
    CHECK(other.training_episodes == 1000);
    CHECK(other.visual_training);
    CHECK(other.best_manual_scores.empty());
    CHECK(other.best_ai_scores.empty());
}

void test_record_score()
{
    std::vector<int> scores;
    CHECK(format_scores(scores).empty());
    for (const int score : {3, 9, 1, 9, 4, 7, 2})
        record_score(scores, score);
    CHECK((scores == std::vector<int>{9, 9, 7, 4, 3}));
    CHECK(format_scores(scores) == "9, 9, 7, 4, 3");
    record_score(scores, 0);
    CHECK((scores == std::vector<int>{9, 9, 7, 4, 3}));
    record_score(scores, 10);
    CHECK((scores == std::vector<int>{10, 9, 9, 7, 4}));
}
} // namespace

int main()
{
    test_main_menu_and_manual_game();
    test_ai_views();
    test_exit_and_input_filtering();
    test_every_page();
    test_session_state();
    test_record_score();
    return TEST_RESULT();
}
