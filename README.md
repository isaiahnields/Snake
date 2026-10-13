# Snake
Cross-platform Snake game with trainable AI players

## Linting

The lint workflow checks C++ formatting with `clang-format`, analyzes native C++
targets with `clang-tidy`, and runs Android's Gradle lint checks for Java and
Android resources.

To run the native checks locally, install `clang-format` 23.1.3 and `clang-tidy`,
then configure and build with:

```sh
cmake -S . -B build-lint -DCMAKE_BUILD_TYPE=Debug -DSNAKE_ENABLE_CLANG_TIDY=ON
cmake --build build-lint --parallel
```

Check C++ formatting with:

```sh
git ls-files -z -- '*.cpp' '*.h' | xargs -0 clang-format --dry-run --Werror
```

In PowerShell, use:

```powershell
$files = git ls-files -- '*.cpp' '*.h'
clang-format --dry-run --Werror $files
```

To run Android lint checks, install JDK 17 and the Android SDK required by the
project, then run `./gradlew --no-daemon lintDebug` from the `android` directory
(`gradlew.bat` on Windows).
