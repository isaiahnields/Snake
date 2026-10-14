#include "../src/cognito_client.h"

#include <deque>
#include <vector>

#include "test_helpers.h"

namespace
{
CognitoResponse signed_in_response()
{
    CognitoResponse response;
    response.has_tokens = true;
    response.access_token = "access";
    response.id_token = "id";
    response.refresh_token = "refresh";
    response.expires_in = 3600;
    return response;
}

struct MockTransport
{
    std::deque<CognitoResponse> responses;
    std::vector<CognitoRequest> requests;

    CognitoResponse operator()(const CognitoRequest &request)
    {
        requests.push_back(request);
        if (responses.empty()) throw std::runtime_error("Unexpected Cognito request.");
        const auto response = responses.front();
        responses.pop_front();
        return response;
    }
};

void test_requests_and_results()
{
    MockTransport mock;
    CognitoResponse unconfirmed;
    unconfirmed.user_confirmed = false;
    CognitoResponse mfa;
    mfa.challenge_name = "SOFTWARE_TOKEN_MFA";
    mfa.challenge_username = "canonical-user";
    mfa.session = "challenge-session";
    mock.responses = {unconfirmed, {}, {}, signed_in_response(), mfa, signed_in_response(), {}, {}};
    CognitoClient client([&mock](const CognitoRequest &request) { return mock(request); });

    CHECK(client.sign_up("player@example.com", "Password1234").step ==
          AuthStep::ConfirmationRequired);
    CHECK(client.confirm("player@example.com", "123456").step == AuthStep::AccountConfirmed);
    CHECK(client.resend("player@example.com").step == AuthStep::CodeSent);

    const auto signed_in = client.sign_in("player@example.com", "Password1234");
    CHECK(signed_in.step == AuthStep::SignedIn);
    CHECK(signed_in.tokens.email == "player@example.com");
    CHECK(signed_in.tokens.access_token == "access");
    CHECK(signed_in.tokens.id_token == "id");
    CHECK(signed_in.tokens.refresh_token == "refresh");
    CHECK(signed_in.tokens.expires_in == 3600);

    const auto challenge = client.sign_in("player@example.com", "Password1234");
    CHECK(challenge.step == AuthStep::MfaRequired);
    CHECK(challenge.challenge_session == "challenge-session");
    CHECK(challenge.challenge_username == "canonical-user");
    CHECK(client
              .answer_mfa("player@example.com", challenge.challenge_username,
                          challenge.challenge_session, "654321")
              .step == AuthStep::SignedIn);
    CHECK(client.forgot_password("player@example.com").step == AuthStep::CodeSent);
    CHECK(client.reset_password("player@example.com", "234567", "NewPassword1234").step ==
          AuthStep::PasswordReset);

    CHECK(mock.requests.size() == 8);
    CHECK(mock.requests[0].operation == CognitoOperation::SignUp);
    CHECK(mock.requests[0].username == "player@example.com");
    CHECK(mock.requests[0].password == "Password1234");
    CHECK(mock.requests[1].operation == CognitoOperation::ConfirmSignUp);
    CHECK(mock.requests[1].code == "123456");
    CHECK(mock.requests[2].operation == CognitoOperation::ResendConfirmationCode);
    CHECK(mock.requests[3].operation == CognitoOperation::InitiateAuth);
    CHECK(mock.requests[4].operation == CognitoOperation::InitiateAuth);
    CHECK(mock.requests[5].operation == CognitoOperation::RespondToAuthChallenge);
    CHECK(mock.requests[5].username == "canonical-user");
    CHECK(mock.requests[5].session == "challenge-session");
    CHECK(mock.requests[5].code == "654321");
    CHECK(mock.requests[6].operation == CognitoOperation::ForgotPassword);
    CHECK(mock.requests[7].operation == CognitoOperation::ConfirmForgotPassword);
    CHECK(mock.requests[7].code == "234567");
    CHECK(mock.requests[7].password == "NewPassword1234");
}

void test_mfa_fallback_and_invalid_responses()
{
    CognitoResponse mfa;
    mfa.challenge_name = "SOFTWARE_TOKEN_MFA";
    mfa.session = "challenge";
    CognitoResponse unsupported_challenge;
    unsupported_challenge.challenge_name = "NEW_PASSWORD_REQUIRED";
    CognitoResponse no_tokens;
    CognitoResponse expired_tokens = signed_in_response();
    expired_tokens.expires_in = 86401;
    std::deque<CognitoResponse> responses = {mfa, unsupported_challenge, no_tokens, expired_tokens};
    CognitoClient client(
        [&responses](const CognitoRequest &)
        {
            const auto response = responses.front();
            responses.pop_front();
            return response;
        });

    const auto challenge = client.sign_in("player@example.com", "Password1234");
    CHECK(challenge.step == AuthStep::MfaRequired);
    CHECK(challenge.challenge_username == "player@example.com");

    int exceptions = 0;
    try
    {
        client.sign_in("player@example.com", "Password1234");
    }
    catch (const std::runtime_error &)
    {
        ++exceptions;
    }
    try
    {
        client.sign_in("player@example.com", "Password1234");
    }
    catch (const std::runtime_error &)
    {
        ++exceptions;
    }
    try
    {
        client.sign_in("player@example.com", "Password1234");
    }
    catch (const std::runtime_error &)
    {
        ++exceptions;
    }
    CHECK(exceptions == 3);
}

void test_service_errors()
{
    CognitoResponse missing_user;
    missing_user.error = "UserNotFoundException";
    CognitoResponse unverified;
    unverified.error = "InvalidParameterException";
    CognitoResponse unauthorized;
    unauthorized.error = "NotAuthorizedException";
    std::deque<CognitoResponse> responses = {missing_user, unverified, unauthorized};
    CognitoClient client(
        [&responses](const CognitoRequest &)
        {
            const auto response = responses.front();
            responses.pop_front();
            return response;
        });

    int errors = 0;
    try
    {
        client.forgot_password("missing@example.com");
    }
    catch (const CognitoError &error)
    {
        CHECK(error.code == "UserNotFoundException");
        CHECK(std::string(error.what()).find("No account") != std::string::npos);
        ++errors;
    }
    try
    {
        client.forgot_password("unverified@example.com");
    }
    catch (const CognitoError &error)
    {
        CHECK(error.code == "InvalidParameterException");
        CHECK(std::string(error.what()).find("not been verified") != std::string::npos);
        ++errors;
    }
    try
    {
        client.reset_password("player@example.com", "123456", "Password1234");
    }
    catch (const CognitoError &error)
    {
        CHECK(error.code == "NotAuthorizedException");
        CHECK(std::string(error.what()).find("cannot be reset") != std::string::npos);
        ++errors;
    }
    CHECK(errors == 3);
}
} // namespace

int main()
{
    test_requests_and_results();
    test_mfa_fallback_and_invalid_responses();
    test_service_errors();
    return TEST_RESULT();
}
