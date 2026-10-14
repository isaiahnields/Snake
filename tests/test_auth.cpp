#include "test_helpers.h"

#include "../src/views/sign_in_view.h"
#include "../src/views/sign_up_view.h"

#include <deque>
#include <imgui.h>
#include <nlohmann/json.hpp>
#include <utility>
#include <vector>

namespace
{
using Json = nlohmann::json;
const std::string TOKENS =
    R"({"AuthenticationResult":{"AccessToken":"access","IdToken":"id","RefreshToken":"refresh","ExpiresIn":3600}})";

struct MockCognito
{
    std::deque<CognitoHttpResponse> responses;
    std::vector<std::pair<std::string, Json>> requests;
    std::shared_ptr<CognitoClient> client()
    {
        return std::make_shared<CognitoClient>(
            [this](const std::string &operation, const std::string &body)
            {
                requests.emplace_back(operation, Json::parse(body));
                if (responses.empty()) throw std::runtime_error("Unexpected Cognito request.");
                auto response = responses.front();
                responses.pop_front();
                return response;
            });
    }
};

void drain(ViewManager &manager, AuthFormView &form)
{
    const Uint64 deadline = SDL_GetTicks() + 3000;
    while (form.busy() && SDL_GetTicks() < deadline)
    {
        manager.update();
        SDL_Delay(1);
    }
    CHECK(!form.busy());
}

void press(ViewManager &manager, SDL_Keycode key)
{
    SDL_Event event{};
    event.type = SDL_EVENT_KEY_DOWN;
    event.key.key = key;
    manager.handle_event(event);
}

void test_client_requests()
{
    MockCognito mock;
    mock.responses = {
        {200, R"({"UserConfirmed":false})"},
        {200, "{}"},
        {200, "{}"},
        {200, TOKENS},
        {200,
         R"({"ChallengeName":"SOFTWARE_TOKEN_MFA","Session":"challenge","ChallengeParameters":{"USERNAME":"canonical-user"}})"},
        {200, TOKENS}};
    auto client = mock.client();
    CHECK(client->sign_up("player@example.com", "Password1234").step ==
          AuthStep::ConfirmationRequired);
    CHECK(client->confirm("player@example.com", "123456").step == AuthStep::AccountConfirmed);
    CHECK(client->resend("player@example.com").step == AuthStep::CodeSent);
    const auto result = client->sign_in("player@example.com", "Password1234");
    CHECK(result.step == AuthStep::SignedIn);
    CHECK(result.tokens.email == "player@example.com");
    CHECK(result.tokens.access_token == "access" && result.tokens.id_token == "id");
    CHECK(result.tokens.refresh_token == "refresh" && result.tokens.expires_in == 3600);
    const auto challenge = client->sign_in("player@example.com", "Password1234");
    CHECK(challenge.step == AuthStep::MfaRequired);
    CHECK(challenge.challenge_session == "challenge");
    CHECK(challenge.challenge_username == "canonical-user");
    CHECK(client
              ->answer_mfa("player@example.com", challenge.challenge_username,
                           challenge.challenge_session, "654321")
              .step == AuthStep::SignedIn);
    CHECK(mock.requests.size() == 6);
    for (const auto &request : mock.requests)
        CHECK(request.second.at("ClientId") == "5eg3eaf2m25t2eo8ole12nf788");
    CHECK(mock.requests[0].first == "SignUp");
    CHECK(mock.requests[0].second.at("Username") == "player@example.com");
    CHECK(mock.requests[0].second.at("Password") == "Password1234");
    CHECK(mock.requests[0].second.at("UserAttributes")[0].at("Name") == "email");
    CHECK(mock.requests[0].second.at("UserAttributes")[0].at("Value") == "player@example.com");
    CHECK(mock.requests[1].first == "ConfirmSignUp");
    CHECK(mock.requests[1].second.at("ConfirmationCode") == "123456");
    CHECK(mock.requests[2].first == "ResendConfirmationCode");
    CHECK(mock.requests[3].first == "InitiateAuth");
    CHECK(mock.requests[3].second.at("AuthFlow") == "USER_PASSWORD_AUTH");
    CHECK(mock.requests[3].second.at("AuthParameters").at("PASSWORD") == "Password1234");
    CHECK(mock.requests[5].first == "RespondToAuthChallenge");
    CHECK(mock.requests[5].second.at("ChallengeResponses").at("USERNAME") == "canonical-user");
    CHECK(mock.requests[5].second.at("Session") == "challenge");
    CHECK(mock.requests[5].second.at("ChallengeResponses").at("SOFTWARE_TOKEN_MFA_CODE") ==
          "654321");
}

void test_sign_up_and_gate()
{
    MockCognito mock;
    mock.responses = {{200, R"({"UserConfirmed":false})"}, {200, "{}"}, {200, "{}"}, {200, TOKENS}};
    ViewManager manager(nullptr, mock.client());
    CHECK(manager.active_id() == ViewId::SignUpOrSignIn);
    manager.open_page(MenuPage::Play);
    CHECK(manager.active_id() == ViewId::SignUpOrSignIn);
    press(manager, SDLK_1);
    CHECK(manager.active_id() == ViewId::SignUp);
    auto &sign_up = static_cast<SignUpView &>(manager.active());
    sign_up.submit(manager, "player@example.com", "Password1234");
    CHECK(sign_up.busy());
    sign_up.submit(manager, "player@example.com", "Password1234");
    drain(manager, sign_up);
    CHECK(mock.requests.size() == 1);
    CHECK(sign_up.stage() == AuthFormStage::VerifyEmail);
    CHECK(!manager.session().signed_in());
    sign_up.resend_code();
    drain(manager, sign_up);
    CHECK(sign_up.stage() == AuthFormStage::VerifyEmail);
    sign_up.submit_code(manager, "123456");
    drain(manager, sign_up);
    CHECK(manager.active_id() == ViewId::SignIn);
    CHECK(!manager.session().signed_in());
    auto &sign_in = static_cast<SignInView &>(manager.active());
    CHECK(!sign_in.notice().empty());
    sign_in.submit(manager, "player@example.com", "Password1234");
    drain(manager, sign_in);
    CHECK(manager.active_id() == ViewId::MainMenu);
    CHECK(manager.session().signed_in());
    CHECK(manager.session().player.email == "player@example.com");
    manager.session().token_expires_at = SDL_GetTicks();
    manager.update();
    CHECK(manager.active_id() == ViewId::SignIn);
    CHECK(manager.session().player.access_token.empty());
    CHECK(sign_in.notice() == "Your session expired. Please sign in again.");
}

void test_validation()
{
    MockCognito mock;
    ViewManager manager(nullptr, mock.client());
    manager.open(ViewId::SignUp);
    auto &form = static_cast<SignUpView &>(manager.active());
    for (const auto &email : {"", "missing-at", "@domain", "user@", "a@b@c", "a @b"})
    {
        form.submit(manager, email, "Password1234");
        CHECK(!form.busy() && !form.notice().empty());
    }
    for (const auto &password : {"", "Password123", "password1234", "PASSWORD1234", "PasswordOnly"})
    {
        form.submit(manager, "player@example.com", password);
        CHECK(!form.busy() && !form.notice().empty());
    }
    form.submit(manager, "player@example.com", std::string(257, 'a') + "A1");
    CHECK(!form.busy());
    form.submit_code(manager, "123456");
    CHECK(!form.busy());
    form.resend_code();
    CHECK(!form.busy() && !form.notice().empty());
    CHECK(mock.requests.empty());
    mock.responses = {{200, R"({"UserConfirmed":true})"}};
    form.submit(manager, "player@example.com", std::string(254, 'a') + "A1");
    drain(manager, form);
    CHECK(mock.requests.size() == 1);
    CHECK(mock.requests[0].second.at("Password").get<std::string>().size() == 256);
    manager.open(ViewId::SignUp);
    press(manager, SDLK_ESCAPE);
    CHECK(manager.active_id() == ViewId::SignUpOrSignIn);
    press(manager, SDLK_2);
    CHECK(manager.active_id() == ViewId::SignIn);
    press(manager, SDLK_ESCAPE);
    press(manager, SDLK_ESCAPE);
    CHECK(manager.quit_requested());
}

void test_rendered_forms_accept_text()
{
    CHECK(SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy"));
    CHECK(SDL_Init(SDL_INIT_VIDEO));
    SDL_Window *window = nullptr;
    SDL_Renderer *renderer = nullptr;
    CHECK(SDL_CreateWindowAndRenderer("Auth form test", 800, 800, SDL_WINDOW_HIDDEN, &window,
                                      &renderer));
    if (renderer != nullptr)
    {
        MockCognito mock;
        mock.responses = {{200, TOKENS}};
        {
            ViewManager manager(renderer, mock.client());
            CHECK(manager.initialize_ui());
            CHECK(manager.render());
            CHECK(ImGui::GetDrawData()->TotalVtxCount > 0);
            manager.open(ViewId::SignUp);
            CHECK(manager.render());
            manager.open(ViewId::SignIn);
            CHECK(manager.render());
            const auto key = [&](SDL_Keycode value)
            {
                SDL_Event event{};
                event.type = SDL_EVENT_KEY_DOWN;
                event.key.windowID = SDL_GetWindowID(window);
                event.key.key = value;
                manager.handle_event(event);
                manager.render();
                event.type = SDL_EVENT_KEY_UP;
                manager.handle_event(event);
                manager.render();
            };
            const auto type = [&](const char *text)
            {
                SDL_Event event{};
                event.type = SDL_EVENT_TEXT_INPUT;
                event.text.windowID = SDL_GetWindowID(window);
                event.text.text = text;
                manager.handle_event(event);
                manager.render();
            };
            key(SDLK_TAB);
            type("player@example.com");
            key(SDLK_TAB);
            type("Password1234");
            key(SDLK_RETURN);
            auto &form = static_cast<SignInView &>(manager.view(ViewId::SignIn));
            drain(manager, form);
            CHECK(mock.requests.size() == 1);
            if (!mock.requests.empty())
            {
                CHECK(mock.requests[0].second.at("AuthParameters").at("USERNAME") ==
                      "player@example.com");
                CHECK(mock.requests[0].second.at("AuthParameters").at("PASSWORD") ==
                      "Password1234");
            }
            CHECK(manager.session().signed_in());
        }
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
    }
    SDL_Quit();
}

void test_unconfirmed_and_mfa()
{
    MockCognito mock;
    mock.responses = {
        {400, R"({"__type":"UserNotConfirmedException"})"},
        {200, "{}"},
        {200,
         R"({"ChallengeName":"SOFTWARE_TOKEN_MFA","Session":"challenge","ChallengeParameters":{"USERNAME":"canonical"}})"},
        {400, R"({"__type":"CodeMismatchException"})"},
        {200, TOKENS}};
    ViewManager manager(nullptr, mock.client());
    manager.open(ViewId::SignIn);
    auto &form = static_cast<SignInView &>(manager.active());
    form.submit(manager, "player@example.com", "Password1234");
    drain(manager, form);
    CHECK(form.stage() == AuthFormStage::VerifyEmail);
    form.submit_code(manager, "123456");
    drain(manager, form);
    CHECK(form.stage() == AuthFormStage::Credentials);
    form.submit(manager, "player@example.com", "Password1234");
    drain(manager, form);
    CHECK(form.stage() == AuthFormStage::Mfa);
    CHECK(!manager.session().signed_in());
    form.submit_code(manager, "123");
    CHECK(!form.busy() && !form.notice().empty());
    form.submit_code(manager, "654321");
    drain(manager, form);
    CHECK(form.stage() == AuthFormStage::Mfa);
    CHECK(form.notice() == "Incorrect verification code. Please try again.");
    form.submit_code(manager, "123456");
    drain(manager, form);
    CHECK(manager.session().signed_in());
}

void test_errors_do_not_authenticate()
{
    for (
        const auto &response : std::vector<CognitoHttpResponse>{
            {400, R"({"__type":"com.amazonaws#NotAuthorizedException"})"},
            {400, R"({"__type":"TooManyRequestsException"})"},
            {200, "not json"},
            {200, "{}"},
            {200,
             R"({"AuthenticationResult":{"AccessToken":"access","IdToken":"id","RefreshToken":"refresh","ExpiresIn":0}})"},
            {200, R"({"AuthenticationResult":{"AccessToken":"access","ExpiresIn":3600}})"},
            {200, R"({"ChallengeName":"NEW_PASSWORD_REQUIRED","Session":"challenge"})"}})
    {
        MockCognito mock;
        mock.responses = {response};
        ViewManager manager(nullptr, mock.client());
        manager.open(ViewId::SignIn);
        auto &form = static_cast<SignInView &>(manager.active());
        form.submit(manager, "player@example.com", "Password1234");
        drain(manager, form);
        CHECK(manager.active_id() == ViewId::SignIn);
        CHECK(!manager.session().signed_in() && !form.notice().empty());
    }
    auto failing_client = std::make_shared<CognitoClient>(
        [](const std::string &, const std::string &) -> CognitoHttpResponse
        { throw std::runtime_error("Network unavailable."); });
    ViewManager manager(nullptr, failing_client);
    manager.open(ViewId::SignIn);
    auto &form = static_cast<SignInView &>(manager.active());
    form.submit(manager, "player@example.com", "Password1234");
    drain(manager, form);
    CHECK(form.notice() == "Network unavailable.");
    CHECK(!manager.session().signed_in());
}

void test_abandoned_request_does_not_sign_in()
{
    std::promise<CognitoHttpResponse> response;
    auto ready = response.get_future().share();
    auto client = std::make_shared<CognitoClient>([ready](const std::string &, const std::string &)
                                                  { return ready.get(); });
    ViewManager manager(nullptr, client);
    manager.open(ViewId::SignIn);
    auto &form = static_cast<SignInView &>(manager.active());
    form.submit(manager, "player@example.com", "Password1234");
    CHECK(form.busy());
    manager.open(ViewId::SignUpOrSignIn);
    manager.open(ViewId::SignIn);
    response.set_value({200, TOKENS});
    drain(manager, form);
    CHECK(manager.active_id() == ViewId::SignIn);
    CHECK(!manager.session().signed_in());
}
} // namespace

int main()
{
    test_client_requests();
    test_sign_up_and_gate();
    test_validation();
    test_unconfirmed_and_mfa();
    test_errors_do_not_authenticate();
    test_abandoned_request_does_not_sign_in();
    test_rendered_forms_accept_text();
    return TEST_RESULT();
}
