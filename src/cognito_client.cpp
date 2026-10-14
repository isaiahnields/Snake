#include "cognito_client.h"

#include <nlohmann/json.hpp>
#include <utility>

namespace
{
using Json = nlohmann::json;
constexpr const char *CLIENT_ID = "5eg3eaf2m25t2eo8ole12nf788";

std::string string_field(const Json &object, const char *name)
{
    const auto field = object.find(name);
    if (field == object.end() || !field->is_string() ||
        field->get_ref<const std::string &>().empty())
        throw std::runtime_error("Cognito returned an incomplete response. Please try again.");
    return field->get<std::string>();
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

Json request(const CognitoTransport &transport, const char *operation, Json body)
{
    body["ClientId"] = CLIENT_ID;
    const auto response = transport(operation, body.dump());
    Json json = Json::parse(response.body, nullptr, false);
    if (json.is_discarded() || !json.is_object())
        throw std::runtime_error("Cognito returned an invalid response. Please try again.");
    if (response.status < 200 || response.status >= 300)
    {
        std::string code = "UnknownError";
        const auto type = json.find("__type");
        if (type != json.end() && type->is_string())
        {
            code = type->get<std::string>();
            const auto separator = code.find_last_of('#');
            if (separator != std::string::npos) code.erase(0, separator + 1);
        }
        throw CognitoError(code, error_message(code));
    }
    return json;
}

AuthResult authentication(const Json &json, const std::string &email)
{
    const auto challenge = json.find("ChallengeName");
    if (challenge != json.end())
    {
        const auto name = string_field(json, "ChallengeName");
        if (name != "SOFTWARE_TOKEN_MFA")
            throw std::runtime_error("This account requires an unsupported Cognito challenge (" +
                                     name + "). Complete account setup through Cognito.");
        const auto parameters = json.find("ChallengeParameters");
        std::string username = email;
        if (parameters != json.end())
        {
            if (parameters->contains("USERNAME"))
                username = string_field(*parameters, "USERNAME");
            else if (parameters->contains("USER_ID_FOR_SRP"))
                username = string_field(*parameters, "USER_ID_FOR_SRP");
        }
        return {AuthStep::MfaRequired, {}, string_field(json, "Session"), username};
    }
    const auto auth = json.find("AuthenticationResult");
    if (auth == json.end() || !auth->is_object())
        throw std::runtime_error("Cognito did not return an authentication result.");
    const auto expires = auth->find("ExpiresIn");
    if (expires == auth->end() || !expires->is_number_integer() || *expires <= 0 ||
        *expires > 86400)
        throw std::runtime_error("Cognito returned an invalid token lifetime.");
    PlayerTokens tokens{email, string_field(*auth, "AccessToken"), string_field(*auth, "IdToken"),
                        string_field(*auth, "RefreshToken"), expires->get<int>()};
    return {AuthStep::SignedIn, std::move(tokens), {}, {}};
}

// Password recovery errors need account-specific wording rather than the sign-in defaults.
Json recovery_request(const CognitoTransport &transport, const char *operation, Json body)
{
    try
    {
        return request(transport, operation, std::move(body));
    }
    catch (const CognitoError &error)
    {
        if (error.code == "UserNotFoundException")
            throw CognitoError(error.code, "No account is registered with this email.");
        if (error.code == "InvalidParameterException" && std::string(operation) == "ForgotPassword")
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
    const auto json = request(transport_, "SignUp",
                              {{"Username", email},
                               {"Password", password},
                               {"UserAttributes", {{{"Name", "email"}, {"Value", email}}}}});
    const auto confirmed = json.find("UserConfirmed");
    if (confirmed == json.end() || !confirmed->is_boolean())
        throw std::runtime_error("Cognito did not return account confirmation status.");
    return {confirmed->get<bool>() ? AuthStep::AccountConfirmed : AuthStep::ConfirmationRequired,
            {},
            {},
            {}};
}

AuthResult CognitoClient::confirm(const std::string &email, const std::string &code) const
{
    request(transport_, "ConfirmSignUp", {{"Username", email}, {"ConfirmationCode", code}});
    return {AuthStep::AccountConfirmed, {}, {}, {}};
}

AuthResult CognitoClient::resend(const std::string &email) const
{
    request(transport_, "ResendConfirmationCode", {{"Username", email}});
    return {AuthStep::CodeSent, {}, {}, {}};
}

AuthResult CognitoClient::sign_in(const std::string &email, const std::string &password) const
{
    return authentication(
        request(transport_, "InitiateAuth",
                {{"AuthFlow", "USER_PASSWORD_AUTH"},
                 {"AuthParameters", {{"USERNAME", email}, {"PASSWORD", password}}}}),
        email);
}

AuthResult CognitoClient::answer_mfa(const std::string &email, const std::string &username,
                                     const std::string &session, const std::string &code) const
{
    return authentication(request(transport_, "RespondToAuthChallenge",
                                  {{"ChallengeName", "SOFTWARE_TOKEN_MFA"},
                                   {"Session", session},
                                   {"ChallengeResponses",
                                    {{"USERNAME", username}, {"SOFTWARE_TOKEN_MFA_CODE", code}}}}),
                          email);
}

AuthResult CognitoClient::forgot_password(const std::string &email) const
{
    recovery_request(transport_, "ForgotPassword", {{"Username", email}});
    return {AuthStep::CodeSent, {}, {}, {}};
}

AuthResult CognitoClient::reset_password(const std::string &email, const std::string &code,
                                         const std::string &password) const
{
    recovery_request(transport_, "ConfirmForgotPassword",
                     {{"Username", email}, {"ConfirmationCode", code}, {"Password", password}});
    return {AuthStep::PasswordReset, {}, {}, {}};
}
