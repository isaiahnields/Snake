# Snake
Cross-platform Snake game with planned AI players

Manual play is available. AI profiles, algorithms, training, and testing are
not implemented; their menu options report that they are unavailable. Session
state contains training preferences and score lists, not an AI agent. Scores
and preferences remain in memory only and are discarded when the game exits.

## UI and builds

Menus, buttons, training information, and game status use Dear ImGui with its
SDL3 platform and SDLRenderer3 backends. The game board remains SDL-rendered.
Keyboard shortcuts (arrows, Enter/Space, number keys, and Escape) are unchanged;
menu buttons also support mouse clicks and touch. Buttons activate on release,
and unavailable options still explain that the feature is not implemented.

CMake fetches pinned SDL3 and Dear ImGui dependencies (network access is required
on the first configure). Android uses the bundled SDL3 AAR and fetches the same
ImGui sources for its native build.

```sh
cmake -S . -B build
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

The UI follows the window size, while the game board retains its letterboxed
logical resolution. ImGui settings are not written to disk. Backend resources
are shut down before the SDL renderer and window are destroyed.

## Commit-time linting

To block commits to `main` when lint checks fail, enable the repository's local
Git hook once after cloning:

```sh
git config core.hooksPath .githooks
```

The hook runs the same C++ formatting, clang-tidy, and Android lint checks used
by the GitHub Actions lint workflow. It requires `clang-format`, `clang-tidy`,
CMake, Java, and the Android SDK to be installed and available in the
environment. To ensure the linted files match the commit, tracked working-tree
changes must be staged before committing. Commits on other branches are not
blocked by this hook.
