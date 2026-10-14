#pragma once

#include <functional>
#include <optional>
#include <stdexcept>
#include <string>

// Public client IDs are not secrets.
constexpr const char *COGNITO_CLIENT_ID = "5eg3eaf2m25t2eo8ole12nf788";

enum class CognitoOperation
{
    SignUp,
    ConfirmSignUp,
    ResendConfirmationCode,
    InitiateAuth,
    RespondToAuthChallenge,
    ForgotPassword,
    ConfirmForgotPassword
};

// Only the fields an operation uses are set: username, password, code, session.
struct CognitoRequest
{
    CognitoOperation operation;
    std::string username;
    std::string password;
    std::string code;
    std::string session;
};

// A Cognito service error sets `error` to the exception name. Transport failures throw instead.
struct CognitoResponse
{
    std::string error;
    std::optional<bool> user_confirmed;
    std::string challenge_name;
    std::string challenge_username;
    std::string session;
    bool has_tokens = false;
    std::string access_token;
    std::string id_token;
    std::string refresh_token;
    int expires_in = 0;
};

using CognitoTransport = std::function<CognitoResponse(const CognitoRequest &)>;

CognitoResponse cognito_sdk_post(const CognitoRequest &request);

struct PlayerTokens
{
    std::string email;
    std::string access_token;
    std::string id_token;
    std::string refresh_token;
    int expires_in = 0;
};

enum class AuthStep
{
    ConfirmationRequired,
    AccountConfirmed,
    CodeSent,
    MfaRequired,
    SignedIn,
    PasswordReset
};

struct AuthResult
{
    AuthStep step;
    PlayerTokens tokens;
    std::string challenge_session;
    std::string challenge_username;
};

class CognitoError : public std::runtime_error
{
  public:
    CognitoError(const std::string &code, const std::string &message);
    const std::string code;
};

// Requests always target the production pool's region.
class CognitoClient
{
  public:
    explicit CognitoClient(CognitoTransport transport = cognito_sdk_post);
    AuthResult sign_up(const std::string &email, const std::string &password) const;
    AuthResult confirm(const std::string &email, const std::string &code) const;
    AuthResult resend(const std::string &email) const;
    AuthResult sign_in(const std::string &email, const std::string &password) const;
    AuthResult answer_mfa(const std::string &email, const std::string &username,
                          const std::string &session, const std::string &code) const;
    // Emails a password reset code to a registered account with a verified email.
    AuthResult forgot_password(const std::string &email) const;
    AuthResult reset_password(const std::string &email, const std::string &code,
                              const std::string &password) const;

  private:
    CognitoTransport transport_;
};
