#pragma once

#include "../cognito_client.h"
#include "../view.h"

#include <functional>
#include <future>
#include <memory>
#include <string>

// Carries the account email and emailed code between verification and password reset screens.
struct PasswordResetState
{
    std::string email;
    std::string code;
};

// Shared request, navigation and notice mechanics for verification and password reset screens.
class PasswordResetView : public View
{
  public:
    PasswordResetView(std::shared_ptr<CognitoClient> client,
                      std::shared_ptr<PasswordResetState> state);
    void on_enter(ViewManager &manager) override;
    void on_leave() override;
    void handle_event(ViewManager &manager, const SDL_Event &event) override;
    void update(ViewManager &manager, Uint64 now_ms) override;
    void apply_ui_action(ViewManager &manager) override;

    bool busy() const;
    const std::string &notice() const;
    void set_notice(const std::string &message);
    // Forgets everything typed or received, including the shared reset state.
    void clear_fields();

  protected:
    enum class Action
    {
        None,
        Submit,
        Resend,
        Back
    };
    std::shared_ptr<CognitoClient> client_;
    std::shared_ptr<PasswordResetState> state_;
    std::string notice_;
    mutable Action action_ = Action::None;

    void start(std::function<AuthResult()> request);
    // Draws the Back button and notice, then closes the ImGui::BeginDisabled(busy()) block and
    // the form opened by begin_form_screen.
    void render_footer() const;

    virtual ViewId back_view() const = 0;
    virtual void submit_form(ViewManager &manager) = 0;
    virtual void resend(ViewManager &manager)
    {
        (void)manager;
    }
    virtual void succeeded(ViewManager &manager, const AuthResult &result) = 0;
    virtual void failed(ViewManager &manager, const CognitoError &error);
    // Zeroes the screen's input buffers.
    virtual void clear_inputs() = 0;

  private:
    std::future<AuthResult> pending_;
    bool discard_result_ = false;
};
