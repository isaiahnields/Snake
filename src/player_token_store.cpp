#include "player_token_store.h"

#include <iostream>
#include <nlohmann/json.hpp>
#include <utility>

namespace
{
using Json = nlohmann::json;
constexpr const char *TOKENS_FILE = "player_tokens.json";
constexpr Uint64 STORAGE_READY_TIMEOUT_MS = 2000;

struct StorageCloser
{
    SDL_Storage *storage;
    ~StorageCloser()
    {
        if (storage != nullptr && !SDL_CloseStorage(storage))
            std::cerr << "Token storage close error: " << SDL_GetError() << std::endl;
    }
};

bool read_string(const Json &object, const char *name, std::string &value)
{
    const auto field = object.find(name);
    if (field == object.end() || !field->is_string()) return false;
    value = field->get<std::string>();
    return true;
}
} // namespace

PlayerTokenStore::PlayerTokenStore(StorageOpener open_storage)
    : open_storage_(std::move(open_storage))
{
}

PlayerTokenStore PlayerTokenStore::user_storage(const std::string &org, const std::string &app)
{
    return PlayerTokenStore([org, app]
                            { return SDL_OpenUserStorage(org.c_str(), app.c_str(), 0); });
}

SDL_Storage *PlayerTokenStore::open_ready() const
{
    SDL_Storage *storage = open_storage_ ? open_storage_() : nullptr;
    if (storage == nullptr)
    {
        std::cerr << "Token storage open error: " << SDL_GetError() << std::endl;
        return nullptr;
    }
    const Uint64 deadline = SDL_GetTicks() + STORAGE_READY_TIMEOUT_MS;
    while (!SDL_StorageReady(storage))
    {
        if (SDL_GetTicks() >= deadline)
        {
            std::cerr << "Token storage was not ready in time." << std::endl;
            SDL_CloseStorage(storage);
            return nullptr;
        }
        SDL_Delay(1);
    }
    return storage;
}

std::optional<StoredPlayerTokens> PlayerTokenStore::load() const
{
    StorageCloser storage{open_ready()};
    if (storage.storage == nullptr) return std::nullopt;

    Uint64 length = 0;
    if (!SDL_GetStorageFileSize(storage.storage, TOKENS_FILE, &length) || length == 0)
        return std::nullopt;
    std::string contents(static_cast<std::size_t>(length), '\0');
    if (!SDL_ReadStorageFile(storage.storage, TOKENS_FILE, contents.data(), length))
    {
        std::cerr << "Token storage read error: " << SDL_GetError() << std::endl;
        return std::nullopt;
    }

    const Json json = Json::parse(contents, nullptr, false);
    StoredPlayerTokens stored;
    const auto expires_in = json.is_object() ? json.find("expires_in") : json.end();
    const auto expires_at = json.is_object() ? json.find("expires_at") : json.end();
    if (!json.is_object() || !read_string(json, "email", stored.tokens.email) ||
        !read_string(json, "access_token", stored.tokens.access_token) ||
        !read_string(json, "id_token", stored.tokens.id_token) ||
        !read_string(json, "refresh_token", stored.tokens.refresh_token) ||
        expires_in == json.end() || !expires_in->is_number_integer() || expires_at == json.end() ||
        !expires_at->is_number_integer())
    {
        std::cerr << "Ignoring malformed stored player tokens." << std::endl;
        return std::nullopt;
    }
    stored.tokens.expires_in = expires_in->get<int>();
    stored.expires_at = expires_at->get<SDL_Time>();
    return stored;
}

bool PlayerTokenStore::save(const StoredPlayerTokens &stored) const
{
    StorageCloser storage{open_ready()};
    if (storage.storage == nullptr) return false;

    const std::string contents = Json{{"email", stored.tokens.email},
                                      {"access_token", stored.tokens.access_token},
                                      {"id_token", stored.tokens.id_token},
                                      {"refresh_token", stored.tokens.refresh_token},
                                      {"expires_in", stored.tokens.expires_in},
                                      {"expires_at", stored.expires_at}}
                                     .dump();
    if (!SDL_WriteStorageFile(storage.storage, TOKENS_FILE, contents.data(), contents.size()))
    {
        std::cerr << "Token storage write error: " << SDL_GetError() << std::endl;
        return false;
    }
    return true;
}

bool PlayerTokenStore::clear() const
{
    StorageCloser storage{open_ready()};
    if (storage.storage == nullptr) return false;

    SDL_PathInfo info;
    if (!SDL_GetStoragePathInfo(storage.storage, TOKENS_FILE, &info)) return true;
    if (!SDL_RemoveStoragePath(storage.storage, TOKENS_FILE))
    {
        std::cerr << "Token storage remove error: " << SDL_GetError() << std::endl;
        return false;
    }
    return true;
}
