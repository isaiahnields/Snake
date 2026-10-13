#!/usr/bin/env bash
set -eu

for tool in clang-format clang-tidy cmake java; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        printf 'Required lint tool not found: %s\n' "$tool" >&2
        exit 1
    fi
done

git ls-files -z -- '*.cpp' '*.h' | xargs -0 clang-format --dry-run --Werror

cmake -S . -B build-lint -DCMAKE_BUILD_TYPE=Debug -DSNAKE_ENABLE_CLANG_TIDY=ON
cmake --build build-lint --parallel

(
    cd android
    chmod +x ./gradlew
    ./gradlew --no-daemon lintDebug
)
