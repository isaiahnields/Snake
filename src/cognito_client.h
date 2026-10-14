#pragma once

#include <functional>
#include <stdexcept>
#include <string>

struct CognitoHttpResponse
{
    int status;
    std::string body;
};

using CognitoTransport =
    std::function<CognitoHttpResponse(const std::string &, const std::string &)>;

CognitoHttpResponse cognito_https_post(const std::string &operation, const std::string &body);

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
    SignedIn
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

// Public client IDs are not secrets. Requests always target the production pool's region.
class CognitoClient
{
  public:
    explicit CognitoClient(CognitoTransport transport = cognito_https_post);
    AuthResult sign_up(const std::string &email, const std::string &password) const;
    AuthResult confirm(const std::string &email, const std::string &code) const;
    AuthResult resend(const std::string &email) const;
    AuthResult sign_in(const std::string &email, const std::string &password) const;
    AuthResult answer_mfa(const std::string &email, const std::string &username,
                          const std::string &session, const std::string &code) const;

  private:
    CognitoTransport transport_;
};
