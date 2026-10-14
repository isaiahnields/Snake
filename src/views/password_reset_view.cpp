#include "password_reset_view.h"

#include "../view_manager.h"

#include <chrono>
#include <imgui.h>
#include <system_error>
#include <utility>

PasswordResetView::PasswordResetView(std::shared_ptr<CognitoClient> client,
                                     std::shared_ptr<PasswordResetState> state)
    : client_(std::move(client)), state_(std::move(state))
{
}

bool PasswordResetView::busy() const
{
    return pending_.valid();
}

const std::string &PasswordResetView::notice() const
{
    return notice_;
}

void PasswordResetView::set_notice(const std::string &message)
{
    notice_ = message;
}

void PasswordResetView::clear_fields()
{
    clear_inputs();
    state_->email.clear();
    state_->code.clear();
    notice_.clear();
}

void PasswordResetView::on_enter(ViewManager &manager)
{
    (void)manager;
    discard_result_ = busy();
    notice_ = busy() ? "Waiting for the previous request to finish." : "";
    action_ = Action::None;
}

void PasswordResetView::on_leave()
{
    clear_inputs();
    discard_result_ = true;
    action_ = Action::None;
}

void PasswordResetView::start(std::function<AuthResult()> request)
{
    if (busy()) return;
    notice_ = "Contacting Cognito...";
    discard_result_ = false;
    try
    {
        pending_ = std::async(std::launch::async, std::move(request));
    }
    catch (const std::system_error &)
    {
        notice_ = "Unable to start the authentication request. Please try again.";
    }
}

void PasswordResetView::failed(ViewManager &manager, const CognitoError &error)
{
    (void)manager;
    notice_ = error.what();
}

void PasswordResetView::update(ViewManager &manager, Uint64 now_ms)
{
    (void)now_ms;
    if (!busy() || pending_.wait_for(std::chrono::seconds(0)) != std::future_status::ready) return;
    try
    {
        const auto result = pending_.get();
        if (discard_result_)
            notice_.clear();
        else
            succeeded(manager, result);
    }
    catch (const CognitoError &error)
    {
        if (discard_result_)
            notice_.clear();
        else
            failed(manager, error);
    }
    catch (const std::runtime_error &error)
    {
        notice_ = discard_result_ ? "" : error.what();
    }
}

void PasswordResetView::handle_event(ViewManager &manager, const SDL_Event &event)
{
    if (event.type != SDL_EVENT_KEY_DOWN) return;
    if (event.key.key == SDLK_ESCAPE)
    {
        if (!busy()) manager.open(back_view());
    }
    else if (event.key.key == SDLK_RETURN)
        action_ = Action::Submit;
}

void PasswordResetView::render_footer() const
{
    if (ImGui::Button("Back")) action_ = Action::Back;
    ImGui::EndDisabled();
    ImGui::TextWrapped("%s", notice_.c_str());
    ImGui::End();
}

void PasswordResetView::apply_ui_action(ViewManager &manager)
{
    const auto action = std::exchange(action_, Action::None);
    if (busy()) return;
    if (action == Action::Submit)
        submit_form(manager);
    else if (action == Action::Resend)
        resend(manager);
    else if (action == Action::Back)
        manager.open(back_view());
}
