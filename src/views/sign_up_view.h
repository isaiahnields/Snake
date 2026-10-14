#pragma once

#include "auth_form_view.h"

class SignUpView : public AuthFormView
{
  public:
    explicit SignUpView(std::shared_ptr<CognitoClient> client);
};
