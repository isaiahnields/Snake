#pragma once

#include "cognito_client.h"

#include <SDL3/SDL.h>
#include <functional>
#include <optional>
#include <string>

struct StoredPlayerTokens
{
    PlayerTokens tokens;
    // Wall-clock expiry, because SDL_GetTicks() restarts with every launch.
    SDL_Time expires_at = 0;
};

using StorageOpener = std::function<SDL_Storage *()>;

// Persists the signed-in player's Cognito tokens through SDL_Storage so they survive restarts.
class PlayerTokenStore
{
  public:
    explicit PlayerTokenStore(StorageOpener open_storage);
    // Uses SDL_OpenUserStorage, which resolves to the platform's per-user preferences folder.
    static PlayerTokenStore user_storage(const std::string &org, const std::string &app);

    std::optional<StoredPlayerTokens> load() const;
    bool save(const StoredPlayerTokens &stored) const;
    bool clear() const;

  private:
    StorageOpener open_storage_;
    SDL_Storage *open_ready() const;
};
