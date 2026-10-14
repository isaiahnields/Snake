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
    return ViewId::ForgotPassword;
}

void VerifyAccountView::clear_inputs()
{
    code_.fill(0);
}

// Cognito checks the code together with the new password, so it is only validated locally here.
void VerifyAccountView::submit(ViewManager &manager, const std::string &code)
{
    if (busy()) return;
    code_.fill(0);
    if (state_->email.empty())
    {
        notice_ = "Request a password reset code first.";
        return;
    }
    if (!valid_code(code))
    {
        notice_ = "Enter the numeric code from your email.";
        return;
    }
    state_->code = code;
    manager.open(ViewId::ResetPassword);
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
        notice_ = "Request a password reset code first.";
        return;
    }
    const auto client = client_;
    const auto email = state_->email;
    start([client, email] { return client->forgot_password(email); });
}

void VerifyAccountView::succeeded(ViewManager &manager, const AuthResult &result)
{
    (void)manager;
    (void)result;
    notice_ = "A new code was sent to your email.";
}

void VerifyAccountView::render(SDL_Renderer *renderer) const
{
    (void)renderer;
    begin_form_screen("VERIFY ACCOUNT");
    ImGui::BeginDisabled(busy());
    ImGui::TextWrapped("%s", state_->email.c_str());
    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
    ImGui::TextUnformatted("Password reset code");
    ImGui::InputText("##Code", code_.data(), code_.size(), ImGuiInputTextFlags_CharsDecimal);
    ImGui::PopItemWidth();
    if (ImGui::Button("Verify")) action_ = Action::Submit;
    if (ImGui::Button("Resend code")) action_ = Action::Resend;
    render_footer();
}
