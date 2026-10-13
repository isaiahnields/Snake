#pragma once

#include "session_state.h"
#include "view.h"
#include "views/menu_view.h"

#include <SDL3/SDL.h>
#include <array>
#include <memory>
#include <string>

// Owns every View, tracks which one is active, routes SDL events to it and renders it.
class ViewManager
{
  public:
    // renderer may be null (e.g. in tests); render() is then a no-op and pointer
    // coordinates are passed to views unconverted.
    explicit ViewManager(SDL_Renderer *renderer = nullptr);
    ~ViewManager();
    ViewManager(const ViewManager &) = delete;
    ViewManager &operator=(const ViewManager &) = delete;

    void open(ViewId id);
    // Activates the menu view that owns page and shows that page with an optional notice.
    void open_page(MenuPage page, const std::string &notice = {});

    void handle_event(const SDL_Event &event);
    void update();
    void render();

    int frame_delay_ms() const;
    void request_quit();
    bool quit_requested() const;

    ViewId active_id() const;
    View &active();
    View &view(ViewId id);
    SessionState &session();

  private:
    SDL_Renderer *renderer_;
    SessionState session_;
    std::array<std::unique_ptr<View>, static_cast<std::size_t>(ViewId::Count)> views_;
    ViewId active_ = ViewId::MainMenu;
    bool quit_ = false;
};
