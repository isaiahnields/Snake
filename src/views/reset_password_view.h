#pragma once

#include "password_reset_view.h"

#include <array>

// Sets a new password, entered twice, using the verified reset code.
class ResetPasswordView : public PasswordResetView
{
  public:
    ResetPasswordView(std::shared_ptr<CognitoClient> client,
                      std::shared_ptr<PasswordResetState> state);
    void render(SDL_Renderer *renderer) const override;
    void submit(ViewManager &manager, const std::string &password, const std::string &confirmation);

  protected:
    ViewId back_view() const override;
    void submit_form(ViewManager &manager) override;
    void succeeded(ViewManager &manager, const AuthResult &result) override;
    void failed(ViewManager &manager, const CognitoError &error) override;
    void clear_inputs() override;

  private:
    mutable std::array<char, 1025> password_{};
    mutable std::array<char, 1025> confirmation_{};
};
