#pragma once

#include <SDL3/SDL.h>

class MenuView;
class ViewManager;

enum class ViewId
{
    MainMenu,
    ManualGame,
    AIControlCenter,
    TrainAI,
    TestAI,
    TrainCloudAI,
    HallOfFame,
    Settings,
    ReplayVault,
    SaveAndExit,
    Count
};

constexpr int DEFAULT_FRAME_DELAY_MS = 16;

// A screen of the application. Views issue SDL board and ImGui UI instructions in render(); the
// ViewManager owns the frame (clearing and presenting) and routes input to the active view.
class View
{
  public:
    virtual ~View() = default;

    virtual void on_enter(ViewManager &manager)
    {
        (void)manager;
    }
    virtual void handle_event(ViewManager &manager, const SDL_Event &event) = 0;
    virtual void update(ViewManager &manager, Uint64 now_ms)
    {
        (void)manager;
        (void)now_ms;
    }
    virtual void render(SDL_Renderer *renderer) const = 0;
    virtual int frame_delay_ms() const
    {
        return DEFAULT_FRAME_DELAY_MS;
    }
    virtual MenuView *as_menu()
    {
        return nullptr;
    }
};

// Fires at a fixed interval, starting one interval after the first update it sees.
class StepTimer
{
  public:
    void reset()
    {
        started_ = false;
    }

    bool due(Uint64 now_ms, Uint64 interval_ms)
    {
        if (!started_)
        {
            started_ = true;
            last_ms_ = now_ms;
            return false;
        }
        if (now_ms - last_ms_ < interval_ms) return false;
        last_ms_ = now_ms;
        return true;
    }

  private:
    bool started_ = false;
    Uint64 last_ms_ = 0;
};
