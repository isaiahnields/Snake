#pragma once

#include "password_reset_view.h"

#include <array>

// Asks for the account email and has Cognito email a password reset code.
class ForgotPasswordView : public PasswordResetView
{
  public:
    ForgotPasswordView(std::shared_ptr<CognitoClient> client,
                       std::shared_ptr<PasswordResetState> state);
    void render(SDL_Renderer *renderer) const override;
    void submit(ViewManager &manager, const std::string &email);

  protected:
    ViewId back_view() const override;
    void submit_form(ViewManager &manager) override;
    void succeeded(ViewManager &manager, const AuthResult &result) override;
    void clear_inputs() override;

  private:
    std::string requested_email_;
    mutable std::array<char, 255> email_{};
};
