#include "verify_account_view.h"

#include "../renderer.h"
#include "../view_manager.h"
#include "auth_form_view.h"

#include <imgui.h>
#include <utility>

VerifyAccountView::VerifyAccountView(std::shared_ptr<CognitoClient> client,
                                     std::shared_ptr<PasswordResetState> state)
    : PasswordResetView(std::move(client), std::move(state))
{
}

ViewId VerifyAccountView::back_view() const
{
    return confirming_account_ ? confirmation_back_ : ViewId::ForgotPassword;
}

void VerifyAccountView::begin_confirmation(ViewManager &manager, const std::string &email,
                                           ViewId back)
{
    confirming_account_ = true;
    confirmation_back_ = back;
    state_->email = email;
    state_->code.clear();
    manager.open(ViewId::VerifyAccount);
    if (!busy()) notice_ = "Check your email for a verification code.";
}

void VerifyAccountView::begin_password_reset(ViewManager &manager)
{
    confirming_account_ = false;
    manager.open(ViewId::VerifyAccount);
    if (!busy()) notice_ = "Check your email for a password reset code.";
}

void VerifyAccountView::clear_inputs()
{
    code_.fill(0);
}

void VerifyAccountView::submit(ViewManager &manager, const std::string &code)
{
    if (busy()) return;
    code_.fill(0);
    if (state_->email.empty())
    {
        notice_ = confirming_account_ ? "Submit your email and password first."
                                      : "Request a password reset code first.";
        return;
    }
    if (!valid_code(code))
    {
        notice_ = "Enter the numeric code from your email.";
        return;
    }
    if (confirming_account_)
    {
        const auto client = client_;
        const auto email = state_->email;
        start([client, email, code] { return client->confirm(email, code); });
    }
    else
    {
        // Cognito checks reset codes together with the new password on the next screen.
        state_->code = code;
        manager.open(ViewId::ResetPassword);
    }
}

void VerifyAccountView::submit_form(ViewManager &manager)
{
    submit(manager, code_.data());
}

void VerifyAccountView::resend(ViewManager &manager)
{
    (void)manager;
    if (busy()) return;
    if (state_->email.empty())
    {
        notice_ = confirming_account_ ? "Submit your email and password first."
                                      : "Request a password reset code first.";
        return;
    }
    const auto client = client_;
    const auto email = state_->email;
    const bool confirming = confirming_account_;
    start([client, email, confirming]
          { return confirming ? client->resend(email) : client->forgot_password(email); });
}

void VerifyAccountView::succeeded(ViewManager &manager, const AuthResult &result)
{
    if (confirming_account_ && result.step == AuthStep::AccountConfirmed)
    {
        state_->email.clear();
        state_->code.clear();
        manager.open(ViewId::SignIn);
        static_cast<AuthFormView &>(manager.active())
            .set_notice("Email verified. Sign in to play.");
    }
    else if (result.step == AuthStep::CodeSent)
        notice_ = "A new code was sent to your email.";
    else
        notice_ = "Unexpected verification response. Please try again.";
}

void VerifyAccountView::render(SDL_Renderer *renderer) const
{
    (void)renderer;
    begin_form_screen("VERIFY ACCOUNT");
    ImGui::BeginDisabled(busy());
    ImGui::TextWrapped("%s", state_->email.c_str());
    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
    ImGui::TextUnformatted(confirming_account_ ? "Email verification code" : "Password reset code");
    ImGui::InputText("##Code", code_.data(), code_.size(), ImGuiInputTextFlags_CharsDecimal);
    ImGui::PopItemWidth();
    if (ImGui::Button("Verify")) action_ = Action::Submit;
    if (ImGui::Button("Resend code")) action_ = Action::Resend;
    render_footer();
}
