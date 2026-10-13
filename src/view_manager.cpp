#include "view_manager.h"

#include "views/ai_control_center_view.h"
#include "views/hall_of_fame_view.h"
#include "views/main_menu_view.h"
#include "views/manual_game_view.h"
#include "views/replay_vault_menu.h"
#include "views/save_and_exit_view.h"
#include "views/settings_view.h"
#include "views/test_ai_view.h"
#include "views/train_ai_view.h"
#include "views/train_cloud_ai_view.h"

#include <iostream>

namespace
{
constexpr std::size_t index_of(ViewId id)
{
    return static_cast<std::size_t>(id);
}
} // namespace

ViewManager::ViewManager(SDL_Renderer *renderer) : renderer_(renderer)
{
    views_[index_of(ViewId::MainMenu)] = std::make_unique<MainMenuView>();
    views_[index_of(ViewId::ManualGame)] = std::make_unique<ManualGameView>(session_);
    views_[index_of(ViewId::AIControlCenter)] = std::make_unique<AIControlCenterView>(session_);
    views_[index_of(ViewId::TrainAI)] = std::make_unique<TrainAIView>(session_);
    views_[index_of(ViewId::TestAI)] = std::make_unique<TestAIView>(session_);
    views_[index_of(ViewId::TrainCloudAI)] = std::make_unique<TrainCloudAIView>();
    views_[index_of(ViewId::HallOfFame)] = std::make_unique<HallOfFameView>(session_);
    views_[index_of(ViewId::Settings)] = std::make_unique<SettingsView>();
    views_[index_of(ViewId::ReplayVault)] = std::make_unique<ReplayVaultMenu>();
    views_[index_of(ViewId::SaveAndExit)] = std::make_unique<SaveAndExitView>();
    open(ViewId::MainMenu);
}

ViewManager::~ViewManager() = default;

void ViewManager::open(ViewId id)
{
    active_ = id;
    active().on_enter(*this);
}

void ViewManager::open_page(MenuPage page, const std::string &notice)
{
    for (std::size_t index = 0; index < views_.size(); ++index)
    {
        MenuView *menu = views_[index]->as_menu();
        if (menu == nullptr || !menu->owns(page)) continue;
        open(static_cast<ViewId>(index));
        menu->open(page, notice);
        return;
    }
    std::cerr << "No view owns menu page " << static_cast<int>(page) << std::endl;
}

void ViewManager::handle_event(const SDL_Event &input)
{
    if (input.type == SDL_EVENT_QUIT)
    {
        open_page(MenuPage::Exit);
        return;
    }
    if (input.type == SDL_EVENT_KEY_DOWN && input.key.repeat) return;

    SDL_Event event = input;
    if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN || event.type == SDL_EVENT_FINGER_DOWN)
    {
        // Ignore secondary buttons and the synthetic events SDL generates between mouse and
        // touch, so each physical press is delivered exactly once.
        if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
            (event.button.button != SDL_BUTTON_LEFT || event.button.which == SDL_TOUCH_MOUSEID))
            return;
        if (event.type == SDL_EVENT_FINGER_DOWN && event.tfinger.touchID == SDL_MOUSE_TOUCHID)
            return;
        if (renderer_ != nullptr && !SDL_ConvertEventToRenderCoordinates(renderer_, &event))
        {
            std::cerr << "Input conversion error: " << SDL_GetError() << std::endl;
            return;
        }
    }
    active().handle_event(*this, event);
}

void ViewManager::update()
{
    active().update(*this, SDL_GetTicks());
}

void ViewManager::render()
{
    if (renderer_ == nullptr) return;
    SDL_SetRenderDrawColor(renderer_, 30, 30, 30, 255);
    SDL_RenderClear(renderer_);
    active().render(renderer_);
    SDL_SetRenderScale(renderer_, 1.0f, 1.0f);
    SDL_RenderPresent(renderer_);
}

int ViewManager::frame_delay_ms() const
{
    return views_[index_of(active_)]->frame_delay_ms();
}

void ViewManager::request_quit()
{
    quit_ = true;
}

bool ViewManager::quit_requested() const
{
    return quit_;
}

ViewId ViewManager::active_id() const
{
    return active_;
}

View &ViewManager::active()
{
    return *views_[index_of(active_)];
}

View &ViewManager::view(ViewId id)
{
    return *views_[index_of(id)];
}

SessionState &ViewManager::session()
{
    return session_;
}
