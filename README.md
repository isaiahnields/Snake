# Snake
Cross-platform Snake game with trainable AI players

## Menu navigation

The **SNAKE: AI EVOLUTION** main menu contains Play Game, Hall of Fame,
AI Control Center, Replay Vault, Settings, and Exit Game, with nested screens
for the corresponding options.

Use Up/Down and Enter (or Space), number keys, or click/tap an option.
On the main menu, **0** opens Exit Game. Escape returns to the parent menu;
from gameplay it stops the session and returns to its launch screen.
Closing the window also opens the exit confirmation.

Start New Game launches the existing manual game (arrow-key controls).
The AI Control Center uses the built-in, in-memory DQN agent. On Train Active AI,
activate Episodes to Run to cycle through 1000, 100, and 10 episodes, and activate
Render Mode to switch between visual and headless training. Headless training
skips drawing the board and shows a progress screen (episode, scores, epsilon);
it still runs inside the application, not in a separate background process.
Training finishes after the selected episode count. Test Active AI / Run Visual
Game runs one game and reports its score. Learned weights survive menu navigation
and subsequent training sessions, but are lost when the application closes.
Train Active AI in Cloud is a placeholder; no cloud service is configured.

This update implements the menu layout, not the proposed storage and configuration
systems. Difficulty presets, named profiles, alternative algorithms, editable
hyperparameters, batch benchmarks, recordings, filters, and settings are
explicitly marked **Not implemented**. Hall of Fame shows the best manual and AI
scores from the current session only; Replay Vault shows an empty state rather
than fictional recordings. Displayed profile and recording defaults are
placeholders, not active settings. Save & Quit currently closes the application
without saving weights or other data.

## Architecture

`main.cpp` forwards SDL events to a `ViewManager` (`src/view_manager.h`), which
owns one instance of each `View` (`src/view.h`) and the shared `SessionState`
(DQN agent, training settings, best scores). Each frame the manager updates the
active view, clears the renderer, lets the view issue its draw calls, and
presents. The views live in `src/views/`: `MainMenuView`, `ManualGameView`,
`AIControlCenterView`, `TrainAIView`, `TestAIView`, `TrainCloudAIView`,
`HallOfFameView`, `SettingsView`, `ReplayVaultMenu`, and `SaveAndExitView`.
Menu-style views derive from `MenuView`, and each menu page belongs to exactly
one view.

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
