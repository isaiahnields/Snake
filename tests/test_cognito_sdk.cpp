#include "../src/cognito_sdk.h"
#include "../src/cognito_client.h"

#include "test_helpers.h"

namespace
{
void test_unsupported_operation()
{
    CognitoRequest request{};
    request.operation = static_cast<CognitoOperation>(-1);

    bool threw = false;
    try
    {
        cognito_sdk_post(request);
    }
    catch (const std::invalid_argument &error)
    {
        threw = true;
        CHECK(std::string(error.what()) == "Unsupported Cognito operation.");
    }
    catch (...)
    {
        CHECK(false);
    }
    CHECK(threw);
}
} // namespace

int main()
{
    test_unsupported_operation();
    return TEST_RESULT();
}
