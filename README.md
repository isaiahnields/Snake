# Snake
Cross-platform Snake game with trainable AI players

## Nightly desktop builds

The [Nightly desktop builds workflow](.github/workflows/nightly-builds.yml) builds
Release executables for macOS, Ubuntu, and Windows every day at 06:17 UTC from
the default branch. It can also be started manually from GitHub's Actions tab.
Each build must pass the unit tests before it is uploaded.

Download the `snake-nightly-macos`, `snake-nightly-ubuntu`, or
`snake-nightly-windows` artifact from a successful workflow run. Artifacts are
retained for 14 days. Unzip the artifact, then extract its `.tar.gz` archive;
the executable is inside `build/Release`. The archive preserves executable
permissions on macOS and Ubuntu. SDL is linked statically, so no separate SDL
library is needed; Linux still requires the usual system graphics libraries.
