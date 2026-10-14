#pragma once

#include "password_reset_view.h"

#include <array>

// Collects the password reset code Cognito emailed to the account.
class VerifyAccountView : public PasswordResetView
{
  public:
    VerifyAccountView(std::shared_ptr<CognitoClient> client,
                      std::shared_ptr<PasswordResetState> state);
    void render(SDL_Renderer *renderer) const override;
    void submit(ViewManager &manager, const std::string &code);
    void resend(ViewManager &manager) override;

  protected:
    ViewId back_view() const override;
    void submit_form(ViewManager &manager) override;
    void succeeded(ViewManager &manager, const AuthResult &result) override;
    void clear_inputs() override;

  private:
    mutable std::array<char, 129> code_{};
};
