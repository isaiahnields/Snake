#include "reset_password_view.h"

#include "../renderer.h"
#include "../view_manager.h"
#include "auth_form_view.h"

#include <imgui.h>
#include <utility>

ResetPasswordView::ResetPasswordView(std::shared_ptr<CognitoClient> client,
                                     std::shared_ptr<PasswordResetState> state)
    : PasswordResetView(std::move(client), std::move(state))
{
}

ViewId ResetPasswordView::back_view() const
{
    return ViewId::VerifyAccount;
}

void ResetPasswordView::clear_inputs()
{
    password_.fill(0);
    confirmation_.fill(0);
}

void ResetPasswordView::submit(ViewManager &manager, const std::string &password,
                               const std::string &confirmation)
{
    (void)manager;
    if (busy()) return;
    clear_inputs();
    if (state_->email.empty() || state_->code.empty())
    {
        notice_ = "Verify your password reset code first.";
        return;
    }
    if (!valid_password(password))
    {
        notice_ = "Use 12-256 characters, with uppercase, lowercase and a number.";
        return;
    }
    if (password != confirmation)
    {
        notice_ = "Passwords do not match.";
        return;
    }
    const auto client = client_;
    const auto email = state_->email;
    const auto code = state_->code;
    start([client, email, code, password]
          { return client->reset_password(email, code, password); });
}

void ResetPasswordView::submit_form(ViewManager &manager)
{
    submit(manager, password_.data(), confirmation_.data());
}

void ResetPasswordView::succeeded(ViewManager &manager, const AuthResult &result)
{
    (void)result;
    state_->email.clear();
    state_->code.clear();
    manager.open(ViewId::SignIn);
    static_cast<AuthFormView &>(manager.active())
        .set_notice("Password updated. Sign in with your new password.");
}

void ResetPasswordView::failed(ViewManager &manager, const CognitoError &error)
{
    if (error.code != "CodeMismatchException" && error.code != "ExpiredCodeException")
    {
        PasswordResetView::failed(manager, error);
        return;
    }
    state_->code.clear();
    manager.open(ViewId::VerifyAccount);
    static_cast<PasswordResetView &>(manager.active()).set_notice(error.what());
}

void ResetPasswordView::render(SDL_Renderer *renderer) const
{
    (void)renderer;
    begin_form_screen("RESET PASSWORD");
    ImGui::BeginDisabled(busy());
    ImGui::TextWrapped("%s", state_->email.c_str());
    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
    ImGui::TextUnformatted("New password");
    ImGui::InputText("##Password", password_.data(), password_.size(),
                     ImGuiInputTextFlags_Password);
    ImGui::TextUnformatted("Confirm new password");
    ImGui::InputText("##Confirmation", confirmation_.data(), confirmation_.size(),
                     ImGuiInputTextFlags_Password);
    ImGui::PopItemWidth();
    ImGui::TextWrapped("At least 12 characters, with uppercase, lowercase and a number.");
    if (ImGui::Button("Reset Password")) action_ = Action::Submit;
    render_footer();
}
