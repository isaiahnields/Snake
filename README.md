# Snake
Cross-platform Snake game with trainable AI players

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
