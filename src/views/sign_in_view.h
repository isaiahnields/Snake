#pragma once

#include "auth_form_view.h"

class SignInView : public AuthFormView
{
  public:
    explicit SignInView(std::shared_ptr<CognitoClient> client);
};
