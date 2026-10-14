#include "view_manager.h"

#include "views/ai_control_center_view.h"
#include "views/forgot_password_view.h"
#include "views/hall_of_fame_view.h"
#include "views/main_menu_view.h"
#include "views/manual_game_view.h"
#include "views/replay_vault_menu.h"
#include "views/reset_password_view.h"
#include "views/save_and_exit_view.h"
#include "views/settings_view.h"
#include "views/sign_in_view.h"
#include "views/sign_up_or_sign_in_view.h"
#include "views/sign_up_view.h"
#include "views/train_cloud_ai_view.h"
#include "views/verify_account_view.h"

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
#include <iostream>

namespace
{
constexpr std::size_t index_of(ViewId id)
{
    return static_cast<std::size_t>(id);
}
} // namespace

ViewManager::ViewManager(SDL_Renderer *renderer, std::shared_ptr<CognitoClient> client,
                         std::shared_ptr<const PlayerTokenStore> token_store)
    : renderer_(renderer), token_store_(std::move(token_store))
{
    views_[index_of(ViewId::SignUpOrSignIn)] = std::make_unique<SignUpOrSignInView>();
    views_[index_of(ViewId::SignUp)] = std::make_unique<SignUpView>(client);
    views_[index_of(ViewId::SignIn)] = std::make_unique<SignInView>(client);
    const auto reset = std::make_shared<PasswordResetState>();
    views_[index_of(ViewId::ForgotPassword)] = std::make_unique<ForgotPasswordView>(client, reset);
    views_[index_of(ViewId::VerifyAccount)] = std::make_unique<VerifyAccountView>(client, reset);
    views_[index_of(ViewId::ResetPassword)] = std::make_unique<ResetPasswordView>(client, reset);
    views_[index_of(ViewId::MainMenu)] = std::make_unique<MainMenuView>();
    views_[index_of(ViewId::ManualGame)] = std::make_unique<ManualGameView>(session_);
    views_[index_of(ViewId::AIControlCenter)] = std::make_unique<AIControlCenterView>(session_);
    views_[index_of(ViewId::TrainCloudAI)] = std::make_unique<TrainCloudAIView>();
    views_[index_of(ViewId::HallOfFame)] = std::make_unique<HallOfFameView>(session_);
    views_[index_of(ViewId::Settings)] = std::make_unique<SettingsView>();
    views_[index_of(ViewId::ReplayVault)] = std::make_unique<ReplayVaultMenu>();
    views_[index_of(ViewId::SaveAndExit)] = std::make_unique<SaveAndExitView>();
    open(ViewId::SignUpOrSignIn);
    restore_session();
}

void ViewManager::restore_session()
{
    if (token_store_ == nullptr) return;
    auto stored = token_store_->load();
    if (!stored) return;
    SDL_Time now = 0;
    if (!SDL_GetCurrentTime(&now))
    {
        std::cerr << "Clock error: " << SDL_GetError() << std::endl;
        return;
    }
    if (stored->expires_at <= now || stored->tokens.access_token.empty() ||
        stored->tokens.id_token.empty())
    {
        token_store_->clear();
        return;
    }
    const auto remaining_ms = static_cast<Uint64>(SDL_NS_TO_MS(stored->expires_at - now));
    session_.player = std::move(stored->tokens);
    session_.token_expires_at = SDL_GetTicks() + remaining_ms;
    open_page(MenuPage::Main);
}

void ViewManager::sign_in(PlayerTokens tokens, Uint64 now_ms)
{
    session_.player = std::move(tokens);
    const auto lifetime_ms = static_cast<Uint64>(session_.player.expires_in) * 1000;
    session_.token_expires_at = now_ms + lifetime_ms;
    if (token_store_ == nullptr) return;
    SDL_Time now = 0;
    if (!SDL_GetCurrentTime(&now))
    {
        std::cerr << "Clock error: " << SDL_GetError() << std::endl;
        return;
    }
    token_store_->save({session_.player, now + static_cast<SDL_Time>(SDL_MS_TO_NS(lifetime_ms))});
}

void ViewManager::sign_out()
{
    session_.player = {};
    session_.token_expires_at = 0;
    if (token_store_ != nullptr) token_store_->clear();
}

void ViewManager::log_out()
{
    sign_out();
    session_ = SessionState{};
    static_cast<AuthFormView &>(view(ViewId::SignIn)).clear_fields();
    static_cast<AuthFormView &>(view(ViewId::SignUp)).clear_fields();
    for (const auto id : {ViewId::ForgotPassword, ViewId::VerifyAccount, ViewId::ResetPassword})
        static_cast<PasswordResetView &>(view(id)).clear_fields();
    open(ViewId::SignUpOrSignIn);
}

ViewManager::~ViewManager()
{
    if (ui_context_ == nullptr) return;
    ImGui::SetCurrentContext(ui_context_);
    if (renderer_initialized_) ImGui_ImplSDLRenderer3_Shutdown();
    if (platform_initialized_) ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext(ui_context_);
}

bool ViewManager::initialize_ui()
{
    if (renderer_initialized_) return true;
    if (renderer_ == nullptr || ui_context_ != nullptr)
    {
        std::cerr << "ImGui initialization requires a renderer and may not be retried."
                  << std::endl;
        return false;
    }
    IMGUI_CHECKVERSION();
    ui_context_ = ImGui::CreateContext();
    ImGui::SetCurrentContext(ui_context_);
    ImGui::GetIO().IniFilename = nullptr;
    ImGui::StyleColorsDark();
    ImGui::GetStyle().WindowRounding = 0;
    ImGui::GetStyle().WindowBorderSize = 0;
    SDL_Window *window = SDL_GetRenderWindow(renderer_);
    if (window != nullptr)
    {
        platform_initialized_ = ImGui_ImplSDL3_InitForSDLRenderer(window, renderer_);
        if (!platform_initialized_)
        {
            std::cerr << "ImGui SDL platform initialization error: " << SDL_GetError() << std::endl;
            return false;
        }
    }
    renderer_initialized_ = ImGui_ImplSDLRenderer3_Init(renderer_);
    if (!renderer_initialized_)
    {
        std::cerr << "ImGui SDL renderer initialization error: " << SDL_GetError() << std::endl;
        return false;
    }
    if (!ImGui_ImplSDLRenderer3_CreateDeviceObjects())
    {
        std::cerr << "ImGui font texture creation error: " << SDL_GetError() << std::endl;
        ImGui_ImplSDLRenderer3_Shutdown();
        renderer_initialized_ = false;
        return false;
    }
    return true;
}

void ViewManager::open(ViewId id)
{
    if (id != ViewId::SignUpOrSignIn && id != ViewId::SignUp && id != ViewId::SignIn &&
        id != ViewId::ForgotPassword && id != ViewId::VerifyAccount &&
        id != ViewId::ResetPassword && !session_.signed_in())
        id = ViewId::SignUpOrSignIn;
    active().on_leave();
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
        if (active().as_menu() != menu) return;
        menu->open(page, notice);
        return;
    }
    std::cerr << "No view owns menu page " << static_cast<int>(page) << std::endl;
}

void ViewManager::handle_event(const SDL_Event &input)
{
    if (renderer_initialized_) process_ui_event(input);
    if (input.type == SDL_EVENT_QUIT)
    {
        if (!session_.signed_in())
        {
            request_quit();
            return;
        }
        open_page(MenuPage::Exit);
        return;
    }
    if (input.type == SDL_EVENT_KEY_DOWN && input.key.repeat) return;

    active().handle_event(*this, input);
}

void ViewManager::process_ui_event(const SDL_Event &input)
{
    ImGui::SetCurrentContext(ui_context_);
    SDL_Event event = input;
    if ((event.type == SDL_EVENT_MOUSE_MOTION && event.motion.which == SDL_TOUCH_MOUSEID) ||
        ((event.type == SDL_EVENT_MOUSE_BUTTON_DOWN || event.type == SDL_EVENT_MOUSE_BUTTON_UP) &&
         event.button.which == SDL_TOUCH_MOUSEID))
        return;
    if (event.type == SDL_EVENT_FINGER_DOWN || event.type == SDL_EVENT_FINGER_UP ||
        event.type == SDL_EVENT_FINGER_MOTION || event.type == SDL_EVENT_FINGER_CANCELED)
    {
        const auto finger = event.tfinger;
        if (finger.touchID == SDL_MOUSE_TOUCHID) return;
        if (event.type == SDL_EVENT_FINGER_DOWN && !touch_active_)
        {
            touch_active_ = true;
            touch_finger_ = finger.fingerID;
        }
        if (!touch_active_ || touch_finger_ != finger.fingerID) return;
        const bool motion = event.type == SDL_EVENT_FINGER_MOTION;
        const bool down = event.type == SDL_EVENT_FINGER_DOWN;
        const bool canceled = event.type == SDL_EVENT_FINGER_CANCELED;
        int width = 0, height = 0;
        SDL_Window *window = SDL_GetRenderWindow(renderer_);
        const bool size_ok = window != nullptr
                                 ? SDL_GetWindowSize(window, &width, &height)
                                 : SDL_GetRenderOutputSize(renderer_, &width, &height);
        if (!size_ok)
        {
            std::cerr << "Touch coordinate error: " << SDL_GetError() << std::endl;
            return;
        }
        event = {};
        if (motion)
        {
            event.type = SDL_EVENT_MOUSE_MOTION;
            event.motion.windowID = finger.windowID;
            event.motion.which = SDL_TOUCH_MOUSEID;
            event.motion.x = finger.x * width;
            event.motion.y = finger.y * height;
        }
        else
        {
            event.type = down ? SDL_EVENT_MOUSE_BUTTON_DOWN : SDL_EVENT_MOUSE_BUTTON_UP;
            event.button.windowID = finger.windowID;
            event.button.which = SDL_TOUCH_MOUSEID;
            event.button.button = SDL_BUTTON_LEFT;
            event.button.x = canceled ? -1.0f : finger.x * width;
            event.button.y = canceled ? -1.0f : finger.y * height;
            if (!down) touch_active_ = false;
        }
    }
    if (platform_initialized_)
    {
        // Button events also carry a position, even when no preceding motion was delivered.
        if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN || event.type == SDL_EVENT_MOUSE_BUTTON_UP)
        {
            SDL_Event motion{};
            motion.type = SDL_EVENT_MOUSE_MOTION;
            motion.motion.windowID = event.button.windowID;
            motion.motion.which = event.button.which;
            motion.motion.x = event.button.x;
            motion.motion.y = event.button.y;
            ImGui_ImplSDL3_ProcessEvent(&motion);
        }
        ImGui_ImplSDL3_ProcessEvent(&event);
        return;
    }
    ImGuiIO &io = ImGui::GetIO();
    if (event.type == SDL_EVENT_MOUSE_MOTION)
        io.AddMousePosEvent(event.motion.x, event.motion.y);
    else if ((event.type == SDL_EVENT_MOUSE_BUTTON_DOWN ||
              event.type == SDL_EVENT_MOUSE_BUTTON_UP) &&
             event.button.button == SDL_BUTTON_LEFT)
    {
        io.AddMousePosEvent(event.button.x, event.button.y);
        io.AddMouseButtonEvent(0, event.type == SDL_EVENT_MOUSE_BUTTON_DOWN);
    }
    else if (event.type == SDL_EVENT_MOUSE_WHEEL)
        io.AddMouseWheelEvent(-event.wheel.x, event.wheel.y);
}

void ViewManager::update()
{
    if (!session_.player.access_token.empty() && !session_.signed_in())
    {
        sign_out();
        open(ViewId::SignIn);
        static_cast<AuthFormView &>(active()).set_notice(
            "Your session expired. Please sign in again.");
    }
    active().update(*this, SDL_GetTicks());
}

bool ViewManager::render()
{
    if (renderer_ == nullptr) return true;
    if (!renderer_initialized_)
    {
        std::cerr << "ImGui must be initialized before rendering." << std::endl;
        request_quit();
        return false;
    }
    ImGui::SetCurrentContext(ui_context_);
    ImGui_ImplSDLRenderer3_NewFrame();
    if (platform_initialized_)
        ImGui_ImplSDL3_NewFrame();
    else
    {
        int width = 0, height = 0;
        if (!SDL_GetRenderOutputSize(renderer_, &width, &height))
        {
            std::cerr << "Render size error: " << SDL_GetError() << std::endl;
            request_quit();
            return false;
        }
        ImGui::GetIO().DisplaySize = ImVec2(static_cast<float>(width), static_cast<float>(height));
        ImGui::GetIO().DeltaTime = DEFAULT_FRAME_DELAY_MS / 1000.0f;
    }
    if (ImGui::GetIO().DisplaySize.x <= 0 || ImGui::GetIO().DisplaySize.y <= 0) return true;
    ImGui::NewFrame();
    SDL_SetRenderDrawColor(renderer_, 30, 30, 30, 255);
    SDL_RenderClear(renderer_);
    active().render(renderer_);
    ImGui::Render();
    int width = 0, height = 0;
    SDL_RendererLogicalPresentation presentation;
    if (!SDL_GetRenderLogicalPresentation(renderer_, &width, &height, &presentation))
    {
        std::cerr << "Render presentation query error: " << SDL_GetError() << std::endl;
        request_quit();
        return false;
    }
    // UI uses window coordinates; only the game board uses SDL's letterboxed logical size.
    if (!SDL_SetRenderLogicalPresentation(renderer_, 0, 0, SDL_LOGICAL_PRESENTATION_DISABLED))
    {
        std::cerr << "UI render presentation error: " << SDL_GetError() << std::endl;
        request_quit();
        return false;
    }
    SDL_SetRenderScale(renderer_, 1.0f, 1.0f);
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer_);
    SDL_RenderPresent(renderer_);
    if (!SDL_SetRenderLogicalPresentation(renderer_, width, height, presentation))
    {
        std::cerr << "Game render presentation error: " << SDL_GetError() << std::endl;
        request_quit();
        return false;
    }
    active().apply_ui_action(*this);
    return true;
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
