#include "sign_in_view.h"

#include <utility>

SignInView::SignInView(std::shared_ptr<CognitoClient> client)
    : AuthFormView(false, std::move(client))
{
}
