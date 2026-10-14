#pragma once

#include "password_reset_view.h"

#include <array>

// Confirms new accounts or collects the emailed code for a password reset.
class VerifyAccountView : public PasswordResetView
{
  public:
    VerifyAccountView(std::shared_ptr<CognitoClient> client,
                      std::shared_ptr<PasswordResetState> state);
    void render(SDL_Renderer *renderer) const override;
    void submit(ViewManager &manager, const std::string &code);
    void resend(ViewManager &manager) override;
    void begin_confirmation(ViewManager &manager, const std::string &email, ViewId back);
    void begin_password_reset(ViewManager &manager);

  protected:
    ViewId back_view() const override;
    void submit_form(ViewManager &manager) override;
    void succeeded(ViewManager &manager, const AuthResult &result) override;
    void clear_inputs() override;

  private:
    bool confirming_account_ = false;
    ViewId confirmation_back_ = ViewId::SignUp;
    mutable std::array<char, 129> code_{};
};
