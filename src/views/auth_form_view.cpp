#include "auth_form_view.h"

#include "../renderer.h"
#include "../view_manager.h"
#include "verify_account_view.h"

#include <algorithm>
#include <chrono>
#include <imgui.h>
#include <utility>

bool valid_email(const std::string &email)
{
    const auto at = email.find('@');
    return email.size() >= 3 && email.size() <= 254 && at != std::string::npos && at > 0 &&
           at + 1 < email.size() && email.find('@', at + 1) == std::string::npos &&
           email.find_first_of(" \t\r\n") == std::string::npos;
}

bool valid_password(const std::string &password)
{
    const auto length = SDL_utf8strlen(password.c_str());
    const auto has = [&](char first, char last)
    {
        return std::any_of(password.begin(), password.end(),
                           [=](char c) { return c >= first && c <= last; });
    };
    return length >= 12 && length <= 256 && has('a', 'z') && has('A', 'Z') && has('0', '9');
}

bool valid_code(const std::string &code)
{
    return !code.empty() && code.size() <= 128 &&
           std::all_of(code.begin(), code.end(), [](char c) { return c >= '0' && c <= '9'; });
}

AuthFormView::AuthFormView(bool sign_up, std::shared_ptr<CognitoClient> client)
    : sign_up_(sign_up), client_(std::move(client))
{
}

bool AuthFormView::busy() const
{
    return pending_.valid();
}
AuthFormStage AuthFormView::stage() const
{
    return stage_;
}
const std::string &AuthFormView::notice() const
{
    return notice_;
}
void AuthFormView::set_notice(const std::string &message)
{
    notice_ = message;
}

void AuthFormView::on_enter(ViewManager &manager)
{
    (void)manager;
    stage_ = AuthFormStage::Credentials;
    discard_result_ = busy();
    notice_ = busy() ? "Waiting for the previous request to finish." : "";
    action_ = Action::None;
}

void AuthFormView::on_leave()
{
    password_.fill(0);
    code_.fill(0);
    challenge_session_.clear();
    challenge_username_.clear();
    discard_result_ = true;
    action_ = Action::None;
}

void AuthFormView::clear_fields()
{
    email_.fill(0);
    password_.fill(0);
    code_.fill(0);
    account_email_.clear();
    challenge_session_.clear();
    challenge_username_.clear();
    notice_.clear();
    stage_ = AuthFormStage::Credentials;
}

void AuthFormView::start(std::function<AuthResult()> request)
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

void AuthFormView::submit(ViewManager &manager, const std::string &email,
                          const std::string &password)
{
    (void)manager;
    if (busy()) return;
    password_.fill(0);
    if (!valid_email(email))
    {
        notice_ = "Enter a valid email address (at most 254 bytes).";
        return;
    }
    if (password.empty() || (sign_up_ && !valid_password(password)))
    {
        notice_ = sign_up_ ? "Use 12-256 characters, with uppercase, lowercase and a number."
                           : "Enter your password.";
        return;
    }
    account_email_ = email;
    const auto client = client_;
    const bool sign_up = sign_up_;
    start(
        [client, sign_up, email, password]
        { return sign_up ? client->sign_up(email, password) : client->sign_in(email, password); });
}

void AuthFormView::submit_code(ViewManager &manager, const std::string &code)
{
    (void)manager;
    if (busy()) return;
    code_.fill(0);
    if (stage_ != AuthFormStage::Mfa)
    {
        notice_ = "Submit your email and password first.";
        return;
    }
    if (!valid_code(code) || code.size() != 6)
    {
        notice_ = "Enter the numeric verification code.";
        return;
    }
    const auto client = client_;
    const auto email = account_email_;
    const auto session = challenge_session_;
    const auto username = challenge_username_;
    start(
        [client, email, session, username, code]
        { return client->answer_mfa(email, username, session, code); });
}

void AuthFormView::update(ViewManager &manager, Uint64 now_ms)
{
    if (!busy() || pending_.wait_for(std::chrono::seconds(0)) != std::future_status::ready) return;
    try
    {
        auto result = pending_.get();
        if (discard_result_)
        {
            notice_.clear();
            return;
        }
        if (result.step == AuthStep::SignedIn)
        {
            manager.sign_in(std::move(result.tokens), now_ms);
            manager.open_page(MenuPage::Main);
        }
        else if (result.step == AuthStep::AccountConfirmed)
        {
            manager.open(ViewId::SignIn);
            static_cast<AuthFormView &>(manager.active())
                .set_notice("Email verified. Sign in to play.");
        }
        else if (result.step == AuthStep::MfaRequired)
        {
            stage_ = AuthFormStage::Mfa;
            challenge_session_ = std::move(result.challenge_session);
            challenge_username_ = std::move(result.challenge_username);
            notice_ = "Enter the six-digit code from your authenticator app.";
        }
        else if (result.step == AuthStep::ConfirmationRequired)
        {
            static_cast<VerifyAccountView &>(manager.view(ViewId::VerifyAccount))
                .begin_confirmation(manager, account_email_,
                                    sign_up_ ? ViewId::SignUp : ViewId::SignIn);
        }
        else
        {
            notice_ = "Unexpected authentication response. Please try again.";
        }
    }
    catch (const CognitoError &error)
    {
        if (discard_result_)
        {
            notice_.clear();
            return;
        }
        if (error.code == "UserNotConfirmedException")
        {
            auto &verify =
                static_cast<VerifyAccountView &>(manager.view(ViewId::VerifyAccount));
            verify.begin_confirmation(manager, account_email_,
                                      sign_up_ ? ViewId::SignUp : ViewId::SignIn);
            verify.set_notice(error.what());
        }
        else
            notice_ = error.what();
    }
    catch (const std::runtime_error &error)
    {
        if (discard_result_)
        {
            notice_.clear();
            return;
        }
        notice_ = error.what();
    }
}

void AuthFormView::handle_event(ViewManager &manager, const SDL_Event &event)
{
    if (event.type != SDL_EVENT_KEY_DOWN) return;
    if (event.key.key == SDLK_ESCAPE)
    {
        if (!busy()) manager.open(ViewId::SignUpOrSignIn);
    }
    else if (event.key.key == SDLK_RETURN)
        action_ = Action::Submit;
}

void AuthFormView::render(SDL_Renderer *renderer) const
{
    (void)renderer;
    begin_form_screen(sign_up_ ? "SIGN UP" : "SIGN IN");
    ImGui::BeginDisabled(busy());
    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
    if (stage_ == AuthFormStage::Credentials)
    {
        ImGui::TextUnformatted("Email");
        ImGui::InputText("##Email", email_.data(), email_.size());
        ImGui::TextUnformatted("Password");
        ImGui::InputText("##Password", password_.data(), password_.size(),
                         ImGuiInputTextFlags_Password);
        if (sign_up_)
            ImGui::TextWrapped("At least 12 characters, with uppercase, lowercase and a number.");
    }
    else
    {
        ImGui::TextWrapped("%s", account_email_.c_str());
        ImGui::TextUnformatted("Authenticator code");
        ImGui::InputText("##Code", code_.data(), code_.size(), ImGuiInputTextFlags_CharsDecimal);
    }
    ImGui::PopItemWidth();
    const char *label =
        stage_ == AuthFormStage::Credentials ? (sign_up_ ? "Sign Up" : "Sign In") : "Verify";
    if (ImGui::Button(label)) action_ = Action::Submit;
    if (!sign_up_ && stage_ == AuthFormStage::Credentials && ImGui::Button("Forgot Password"))
        action_ = Action::ForgotPassword;
    if (ImGui::Button(sign_up_ ? "Already have an account? Sign In" : "Need an account? Sign Up"))
        action_ = Action::Switch;
    if (ImGui::Button("Back")) action_ = Action::Back;
    ImGui::EndDisabled();
    ImGui::TextWrapped("%s", notice_.c_str());
    ImGui::End();
}

void AuthFormView::apply_ui_action(ViewManager &manager)
{
    const auto action = std::exchange(action_, Action::None);
    if (busy()) return;
    if (action == Action::Submit)
    {
        if (stage_ == AuthFormStage::Credentials)
            submit(manager, email_.data(), password_.data());
        else
            submit_code(manager, code_.data());
    }
    else if (action == Action::Back)
        manager.open(ViewId::SignUpOrSignIn);
    else if (action == Action::Switch)
        manager.open(sign_up_ ? ViewId::SignIn : ViewId::SignUp);
    else if (action == Action::ForgotPassword)
        manager.open(ViewId::ForgotPassword);
}
