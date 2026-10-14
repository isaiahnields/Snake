# Snake
Cross-platform Snake game with planned AI players

## Player accounts

The game opens with **Sign Up** and **Sign In** and requires authentication before
the main menu. Both forms accept email and password. New players must enter the
verification code Cognito emails them, then sign in. The forms can resend an
email verification code. Existing accounts with software-token MFA enabled are
prompted for their authenticator code.

Accounts use the `snake-prod-players` Cognito pool (`us-west-2_UZ8VHBqRy`) in
`us-west-2`, with public app client `5eg3eaf2m25t2eo8ole12nf788`. No AWS access keys
or client secret are needed. Requests go directly to Cognito over verified HTTPS,
on a background worker. Passwords and tokens are not saved to disk.
Sessions expire after the access-token lifetime and require signing in again;
automatic refresh, password reset and MFA enrollment are not implemented.
Unsupported account-setup challenges display an error rather than granting access.

The pool and client are defined in [cognito.tf](infra/terraform/cognito.tf).
**Deploy the Terraform change enabling `ALLOW_USER_PASSWORD_AUTH` before using
sign-in**; existing SRP and refresh-token flows remain enabled. Passwords require
12-256 characters, including uppercase, lowercase and a number.

Windows uses WinHTTP and Android uses `HttpsURLConnection` with the platform
certificate store. Linux and macOS builds require libcurl development files
(e.g. `libcurl4-openssl-dev` on Ubuntu). CMake fetches the pinned nlohmann/json
dependency. Unit tests use an injected transport, not live player accounts.

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
