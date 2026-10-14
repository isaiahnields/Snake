#include "sign_up_view.h"

#include <utility>

SignUpView::SignUpView(std::shared_ptr<CognitoClient> client)
    : AuthFormView(true, std::move(client))
{
}
