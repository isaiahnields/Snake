#pragma once

#include <cstdio>

inline int& test_failures() {
    static int failures = 0;
    return failures;
}

#define CHECK(cond)                                                              \
    do {                                                                         \
        if (!(cond)) {                                                           \
            std::fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #cond); \
            ++test_failures();                                                   \
        }                                                                        \
    } while (0)

#define TEST_RESULT() (test_failures() == 0 ? 0 : 1)
