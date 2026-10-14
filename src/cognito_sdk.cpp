#include "cognito_sdk.h"
#include "cognito_client.h"

#include <aws/cognito-idp/CognitoIdentityProviderClient.h>
#include <aws/cognito-idp/model/AttributeType.h>
#include <aws/cognito-idp/model/AuthFlowType.h>
#include <aws/cognito-idp/model/ChallengeNameType.h>
#include <aws/cognito-idp/model/ConfirmForgotPasswordRequest.h>
#include <aws/cognito-idp/model/ConfirmSignUpRequest.h>
#include <aws/cognito-idp/model/ForgotPasswordRequest.h>
#include <aws/cognito-idp/model/InitiateAuthRequest.h>
#include <aws/cognito-idp/model/ResendConfirmationCodeRequest.h>
#include <aws/cognito-idp/model/RespondToAuthChallengeRequest.h>
#include <aws/cognito-idp/model/SignUpRequest.h>
#include <aws/core/Aws.h>
#include <aws/core/client/ClientConfiguration.h>

#include <stdexcept>
#include <string>
#include <utility>

namespace
{
namespace Cognito = Aws::CognitoIdentityProvider;
namespace Model = Cognito::Model;

class AwsSdkRuntime
{
  public:
    AwsSdkRuntime()
    {
        Aws::InitAPI(options_);
    }

    ~AwsSdkRuntime()
    {
        Aws::ShutdownAPI(options_);
    }

  private:
    Aws::SDKOptions options_;
};

AwsSdkRuntime &aws_runtime()
{
    static AwsSdkRuntime runtime;
    return runtime;
}

Cognito::CognitoIdentityProviderClient &cognito_client()
{
    (void)aws_runtime();
    static Aws::Client::ClientConfiguration configuration = []
    {
        Aws::Client::ClientConfiguration config;
        config.region = Aws::Region::US_WEST_2;
        config.connectTimeoutMs = 5000;
        config.requestTimeoutMs = 10000;
        return config;
    }();
    static Cognito::CognitoIdentityProviderClient client(configuration);
    return client;
}

Aws::String to_aws(const std::string &value)
{
    return Aws::String(value.c_str(), value.size());
}

std::string standard_string(const Aws::String &value)
{
    return std::string(value.c_str(), value.size());
}

template <typename Outcome> CognitoResponse failure(const Outcome &outcome)
{
    CognitoResponse response;
    response.error = standard_string(
        outcome.GetError().GetExceptionName());
    if (response.error.empty()) response.error = "UnknownError";
    return response;
}

template <typename Result> CognitoResponse authentication_response(const Result &result)
{
    CognitoResponse response;
    const auto challenge =
        Model::ChallengeNameTypeMapper::GetNameForChallengeNameType(result.GetChallengeName());
    if (!challenge.empty() && challenge != "NOT_SET")
        response.challenge_name = standard_string(challenge);

    const auto &parameters = result.GetChallengeParameters();
    auto username = parameters.find("USERNAME");
    if (username == parameters.end()) username = parameters.find("USER_ID_FOR_SRP");
    if (username != parameters.end())
        response.challenge_username = standard_string(username->second);

    response.session = standard_string(result.GetSession());
    const auto &auth = result.GetAuthenticationResult();
    if (!auth.GetAccessToken().empty() || !auth.GetIdToken().empty() ||
        !auth.GetRefreshToken().empty())
    {
        response.has_tokens = true;
        response.access_token = standard_string(auth.GetAccessToken());
        response.id_token = standard_string(auth.GetIdToken());
        response.refresh_token = standard_string(auth.GetRefreshToken());
        response.expires_in = auth.GetExpiresIn();
    }
    return response;
}

template <typename Request> Request make_request()
{
    Request request;
    request.SetClientId(COGNITO_CLIENT_ID);
    return request;
}
} // namespace

CognitoResponse cognito_sdk_post(const CognitoRequest &input)
{
    switch (input.operation)
    {
    case CognitoOperation::SignUp:
    case CognitoOperation::ConfirmSignUp:
    case CognitoOperation::ResendConfirmationCode:
    case CognitoOperation::InitiateAuth:
    case CognitoOperation::RespondToAuthChallenge:
    case CognitoOperation::ForgotPassword:
    case CognitoOperation::ConfirmForgotPassword:
        break;
    default:
        throw std::invalid_argument("Unsupported Cognito operation.");
    }

    auto &client = cognito_client();
    switch (input.operation)
    {
    case CognitoOperation::SignUp:
    {
        auto request = make_request<Model::SignUpRequest>();
        request.SetUsername(to_aws(input.username));
        request.SetPassword(to_aws(input.password));
        Model::AttributeType email;
        email.SetName("email");
        email.SetValue(to_aws(input.username));
        request.AddUserAttributes(std::move(email));
        const auto outcome = client.SignUp(request);
        if (!outcome.IsSuccess()) return failure(outcome);
        CognitoResponse response;
        response.user_confirmed = outcome.GetResult().GetUserConfirmed();
        return response;
    }
    case CognitoOperation::ConfirmSignUp:
    {
        auto request = make_request<Model::ConfirmSignUpRequest>();
        request.SetUsername(to_aws(input.username));
        request.SetConfirmationCode(to_aws(input.code));
        const auto outcome = client.ConfirmSignUp(request);
        return outcome.IsSuccess() ? CognitoResponse{} : failure(outcome);
    }
    case CognitoOperation::ResendConfirmationCode:
    {
        auto request = make_request<Model::ResendConfirmationCodeRequest>();
        request.SetUsername(to_aws(input.username));
        const auto outcome = client.ResendConfirmationCode(request);
        return outcome.IsSuccess() ? CognitoResponse{} : failure(outcome);
    }
    case CognitoOperation::InitiateAuth:
    {
        auto request = make_request<Model::InitiateAuthRequest>();
        request.SetAuthFlow(Model::AuthFlowType::USER_PASSWORD_AUTH);
        request.AddAuthParameters("USERNAME", to_aws(input.username));
        request.AddAuthParameters("PASSWORD", to_aws(input.password));
        const auto outcome = client.InitiateAuth(request);
        return outcome.IsSuccess() ? authentication_response(outcome.GetResult())
                                   : failure(outcome);
    }
    case CognitoOperation::RespondToAuthChallenge:
    {
        auto request = make_request<Model::RespondToAuthChallengeRequest>();
        request.SetChallengeName(Model::ChallengeNameType::SOFTWARE_TOKEN_MFA);
        request.SetSession(to_aws(input.session));
        request.AddChallengeResponses("USERNAME", to_aws(input.username));
        request.AddChallengeResponses("SOFTWARE_TOKEN_MFA_CODE", to_aws(input.code));
        const auto outcome = client.RespondToAuthChallenge(request);
        return outcome.IsSuccess() ? authentication_response(outcome.GetResult())
                                   : failure(outcome);
    }
    case CognitoOperation::ForgotPassword:
    {
        auto request = make_request<Model::ForgotPasswordRequest>();
        request.SetUsername(to_aws(input.username));
        const auto outcome = client.ForgotPassword(request);
        return outcome.IsSuccess() ? CognitoResponse{} : failure(outcome);
    }
    case CognitoOperation::ConfirmForgotPassword:
    {
        auto request = make_request<Model::ConfirmForgotPasswordRequest>();
        request.SetUsername(to_aws(input.username));
        request.SetConfirmationCode(to_aws(input.code));
        request.SetPassword(to_aws(input.password));
        const auto outcome = client.ConfirmForgotPassword(request);
        return outcome.IsSuccess() ? CognitoResponse{} : failure(outcome);
    }
    }
    throw std::invalid_argument("Unsupported Cognito operation.");
}
