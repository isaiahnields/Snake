#include "cognito_client.h"

#include <utility>

namespace
{
const char *INCOMPLETE_RESPONSE = "Cognito returned an incomplete response. Please try again.";

const std::string &required(const std::string &value)
{
    if (value.empty()) throw std::runtime_error(INCOMPLETE_RESPONSE);
    return value;
}

std::string error_message(const std::string &code)
{
    if (code == "NotAuthorizedException" || code == "UserNotFoundException")
        return "Incorrect email or password.";
    if (code == "UsernameExistsException")
        return "An account with this email already exists. Sign in instead.";
    if (code == "UserNotConfirmedException") return "Verify your email before signing in.";
    if (code == "InvalidPasswordException")
        return "Use at least 12 characters, with uppercase, lowercase and a number.";
    if (code == "CodeMismatchException") return "Incorrect verification code. Please try again.";
    if (code == "ExpiredCodeException") return "The code has expired. Request a new code.";
    if (code == "LimitExceededException" || code == "TooManyRequestsException")
        return "Too many attempts. Please wait and try again.";
    if (code == "PasswordResetRequiredException")
        return "This account requires a password reset through Cognito.";
    if (code == "InvalidParameterException")
        return "Cognito rejected the input. Check your email and password.";
    return "Cognito request failed (" + code + "). Please try again.";
}

CognitoResponse request(const CognitoTransport &transport, CognitoRequest body)
{
    auto response = transport(body);
    if (!response.error.empty()) throw CognitoError(response.error, error_message(response.error));
    return response;
}

AuthResult authentication(const CognitoResponse &response, const std::string &email)
{
    if (!response.challenge_name.empty())
    {
        if (response.challenge_name != "SOFTWARE_TOKEN_MFA")
            throw std::runtime_error("This account requires an unsupported Cognito challenge (" +
                                     response.challenge_name +
                                     "). Complete account setup through Cognito.");
        return {AuthStep::MfaRequired,
                {},
                required(response.session),
                response.challenge_username.empty() ? email : response.challenge_username};
    }
    if (!response.has_tokens)
        throw std::runtime_error("Cognito did not return an authentication result.");
    if (response.expires_in <= 0 || response.expires_in > 86400)
        throw std::runtime_error("Cognito returned an invalid token lifetime.");
    PlayerTokens tokens{email, required(response.access_token), required(response.id_token),
                        required(response.refresh_token), response.expires_in};
    return {AuthStep::SignedIn, std::move(tokens), {}, {}};
}

// Password recovery errors need account-specific wording rather than the sign-in defaults.
CognitoResponse recovery_request(const CognitoTransport &transport, CognitoRequest body)
{
    const auto operation = body.operation;
    try
    {
        return request(transport, std::move(body));
    }
    catch (const CognitoError &error)
    {
        if (error.code == "UserNotFoundException")
            throw CognitoError(error.code, "No account is registered with this email.");
        if (error.code == "InvalidParameterException" &&
            operation == CognitoOperation::ForgotPassword)
            throw CognitoError(error.code, "This email has not been verified, so the password "
                                           "cannot be reset. Verify your email first.");
        if (error.code == "NotAuthorizedException")
            throw CognitoError(error.code, "The password for this account cannot be reset.");
        throw;
    }
}
} // namespace

CognitoError::CognitoError(const std::string &error_code, const std::string &message)
    : std::runtime_error(message), code(error_code)
{
}

CognitoClient::CognitoClient(CognitoTransport transport) : transport_(std::move(transport)) {}

AuthResult CognitoClient::sign_up(const std::string &email, const std::string &password) const
{
    const auto response = request(transport_, {CognitoOperation::SignUp, email, password, {}, {}});
    if (!response.user_confirmed)
        throw std::runtime_error("Cognito did not return account confirmation status.");
    return {*response.user_confirmed ? AuthStep::AccountConfirmed : AuthStep::ConfirmationRequired,
            {},
            {},
            {}};
}

AuthResult CognitoClient::confirm(const std::string &email, const std::string &code) const
{
    request(transport_, {CognitoOperation::ConfirmSignUp, email, {}, code, {}});
    return {AuthStep::AccountConfirmed, {}, {}, {}};
}

AuthResult CognitoClient::resend(const std::string &email) const
{
    request(transport_, {CognitoOperation::ResendConfirmationCode, email, {}, {}, {}});
    return {AuthStep::CodeSent, {}, {}, {}};
}

AuthResult CognitoClient::sign_in(const std::string &email, const std::string &password) const
{
    return authentication(
        request(transport_, {CognitoOperation::InitiateAuth, email, password, {}, {}}), email);
}

AuthResult CognitoClient::answer_mfa(const std::string &email, const std::string &username,
                                     const std::string &session, const std::string &code) const
{
    return authentication(
        request(transport_,
                {CognitoOperation::RespondToAuthChallenge, username, {}, code, session}),
        email);
}

AuthResult CognitoClient::forgot_password(const std::string &email) const
{
    recovery_request(transport_, {CognitoOperation::ForgotPassword, email, {}, {}, {}});
    return {AuthStep::CodeSent, {}, {}, {}};
}

AuthResult CognitoClient::reset_password(const std::string &email, const std::string &code,
                                         const std::string &password) const
{
    recovery_request(transport_,
                     {CognitoOperation::ConfirmForgotPassword, email, password, code, {}});
    return {AuthStep::PasswordReset, {}, {}, {}};
}
