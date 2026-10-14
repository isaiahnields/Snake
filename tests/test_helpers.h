#pragma once

#include <cstdio>

#include "../src/view_manager.h"

inline void authenticate_test_player(ViewManager &manager)
{
    manager.session().player = {"player@example.com", "test-access", "test-id", "test-refresh",
                                3600};
    manager.session().token_expires_at = SDL_GetTicks() + 3600000;
    manager.open(ViewId::MainMenu);
}

inline int &test_failures()
{
    static int failures = 0;
    return failures;
}

#define CHECK(cond)                                                                                \
    do                                                                                             \
    {                                                                                              \
        if (!(cond))                                                                               \
        {                                                                                          \
            std::fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #cond);          \
            ++test_failures();                                                                     \
        }                                                                                          \
    } while (0)

#define TEST_RESULT() (test_failures() == 0 ? 0 : 1)
