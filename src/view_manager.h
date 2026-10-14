#pragma once

#include "gamepad_input.h"
#include "player_token_store.h"
#include "session_state.h"
#include "view.h"
#include "views/menu_view.h"

#include <SDL3/SDL.h>
#include <array>
#include <memory>
#include <string>

struct ImGuiContext;

// Owns every View, tracks which one is active, routes SDL events to it and renders it.
class ViewManager
{
  public:
    // renderer may be null for non-rendering tests. Call initialize_ui() before rendering.
    // When token_store is set, unexpired stored tokens are restored and sign-ins are persisted.
    explicit ViewManager(SDL_Renderer *renderer = nullptr,
                         std::shared_ptr<CognitoClient> client = std::make_shared<CognitoClient>(),
                         std::shared_ptr<const PlayerTokenStore> token_store = nullptr);
    ~ViewManager();
    ViewManager(const ViewManager &) = delete;
    ViewManager &operator=(const ViewManager &) = delete;

    bool initialize_ui();
    void open(ViewId id);
    // Activates the menu view that owns page and shows that page with an optional notice.
    void open_page(MenuPage page, const std::string &notice = {});

    void handle_event(const SDL_Event &event);
    void update();
    bool render();

    int frame_delay_ms() const;
    void request_quit();
    bool quit_requested() const;

    ViewId active_id() const;
    View &active();
    View &view(ViewId id);
    SessionState &session();
    void sign_in(PlayerTokens tokens, Uint64 now_ms);
    void sign_out();
    // Signs out, clears stored tokens and per-player state, and returns to SignUpOrSignIn.
    void log_out();

  private:
    SDL_Renderer *renderer_;
    std::shared_ptr<const PlayerTokenStore> token_store_;
    SessionState session_;
    std::array<std::unique_ptr<View>, static_cast<std::size_t>(ViewId::Count)> views_;
    ViewId active_ = ViewId::SignUpOrSignIn;
    bool quit_ = false;
    ImGuiContext *ui_context_ = nullptr;
    bool platform_initialized_ = false;
    bool renderer_initialized_ = false;
    bool touch_active_ = false;
    SDL_FingerID touch_finger_ = 0;
    GamepadInput gamepad_input_;

    void process_ui_event(const SDL_Event &event);
    void sync_ui_gamepads();
    bool custom_controller_navigation();
    void restore_session();
};
