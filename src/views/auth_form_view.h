#pragma once

#include "../cognito_client.h"
#include "../view.h"

#include <array>
#include <future>
#include <memory>
#include <string>

enum class AuthFormStage
{
    Credentials,
    Mfa
};

bool valid_email(const std::string &email);
// Mirrors the Cognito pool policy: 12-256 characters with uppercase, lowercase and a number.
bool valid_password(const std::string &password);
// Accepts 1-128 digit email verification codes.
bool valid_code(const std::string &code);

// Shared form mechanics for email/password and authenticator challenges.
class AuthFormView : public View
{
  public:
    AuthFormView(bool sign_up, std::shared_ptr<CognitoClient> client);
    void on_enter(ViewManager &manager) override;
    void on_leave() override;
    void handle_event(ViewManager &manager, const SDL_Event &event) override;
    void update(ViewManager &manager, Uint64 now_ms) override;
    void render(SDL_Renderer *renderer) const override;
    void apply_ui_action(ViewManager &manager) override;

    void submit(ViewManager &manager, const std::string &email, const std::string &password);
    void submit_code(ViewManager &manager, const std::string &code);
    bool busy() const;
    AuthFormStage stage() const;
    const std::string &notice() const;
    void set_notice(const std::string &message);
    // Forgets everything typed or received, so the next player starts with an empty form.
    void clear_fields();

  private:
    enum class Action
    {
        None,
        Submit,
        Back,
        Switch,
        ForgotPassword
    };
    const bool sign_up_;
    std::shared_ptr<CognitoClient> client_;
    AuthFormStage stage_ = AuthFormStage::Credentials;
    std::string notice_;
    std::string account_email_;
    std::string challenge_session_;
    std::string challenge_username_;
    std::future<AuthResult> pending_;
    bool discard_result_ = false;
    mutable std::array<char, 255> email_{};
    mutable std::array<char, 1025> password_{};
    mutable std::array<char, 129> code_{};
    mutable Action action_ = Action::None;

    void start(std::function<AuthResult()> request);
};
