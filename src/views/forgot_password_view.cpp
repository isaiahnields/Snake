#include "forgot_password_view.h"

#include "../renderer.h"
#include "../view_manager.h"
#include "auth_form_view.h"

#include <imgui.h>
#include <utility>

ForgotPasswordView::ForgotPasswordView(std::shared_ptr<CognitoClient> client,
                                       std::shared_ptr<PasswordResetState> state)
    : PasswordResetView(std::move(client), std::move(state))
{
}

ViewId ForgotPasswordView::back_view() const
{
    return ViewId::SignIn;
}

void ForgotPasswordView::clear_inputs()
{
    email_.fill(0);
    requested_email_.clear();
}

void ForgotPasswordView::submit(ViewManager &manager, const std::string &email)
{
    (void)manager;
    if (busy()) return;
    if (!valid_email(email))
    {
        notice_ = "Enter a valid email address (at most 254 bytes).";
        return;
    }
    requested_email_ = email;
    const auto client = client_;
    start([client, email] { return client->forgot_password(email); });
}

void ForgotPasswordView::submit_form(ViewManager &manager)
{
    submit(manager, email_.data());
}

void ForgotPasswordView::succeeded(ViewManager &manager, const AuthResult &result)
{
    (void)result;
    state_->email = std::exchange(requested_email_, {});
    state_->code.clear();
    manager.open(ViewId::VerifyAccount);
    static_cast<PasswordResetView &>(manager.active())
        .set_notice("Check your email for a password reset code.");
}

void ForgotPasswordView::render(SDL_Renderer *renderer) const
{
    (void)renderer;
    begin_form_screen("FORGOT PASSWORD");
    ImGui::BeginDisabled(busy());
    ImGui::TextWrapped("Enter your account email to receive a password reset code.");
    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
    ImGui::TextUnformatted("Email");
    ImGui::InputText("##Email", email_.data(), email_.size());
    ImGui::PopItemWidth();
    if (ImGui::Button("Send Code")) action_ = Action::Submit;
    render_footer();
}
