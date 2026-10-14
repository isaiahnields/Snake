#include "player_token_store.h"

#include <iostream>
#include <limits>
#include <map>
#include <sstream>
#include <utility>

namespace
{
constexpr const char *TOKENS_FILE = "player_tokens.txt";
// Earlier versions stored JSON here; it is only deleted, never read.
constexpr const char *LEGACY_TOKENS_FILE = "player_tokens.json";
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

// The file holds one "key=value" line per field. Values never contain line breaks.
bool parse_fields(const std::string &contents, std::map<std::string, std::string> &fields)
{
    std::istringstream lines(contents);
    std::string line;
    while (std::getline(lines, line))
    {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;
        const auto separator = line.find('=');
        if (separator == std::string::npos || separator == 0) return false;
        if (!fields.emplace(line.substr(0, separator), line.substr(separator + 1)).second)
            return false;
    }
    return true;
}

bool read_string(const std::map<std::string, std::string> &fields, const char *name,
                 std::string &value)
{
    const auto field = fields.find(name);
    if (field == fields.end() || field->second.empty()) return false;
    value = field->second;
    return true;
}

bool read_integer(const std::map<std::string, std::string> &fields, const char *name,
                  long long &value)
{
    const auto field = fields.find(name);
    if (field == fields.end() || field->second.empty()) return false;
    std::size_t used = 0;
    try
    {
        value = std::stoll(field->second, &used);
    }
    catch (const std::exception &)
    {
        return false;
    }
    return used == field->second.size();
}

bool has_line_break(const std::string &value)
{
    return value.find_first_of("\r\n") != std::string::npos;
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

    std::map<std::string, std::string> fields;
    StoredPlayerTokens stored;
    long long expires_in = 0;
    long long expires_at = 0;
    if (!parse_fields(contents, fields) || !read_string(fields, "email", stored.tokens.email) ||
        !read_string(fields, "access_token", stored.tokens.access_token) ||
        !read_string(fields, "id_token", stored.tokens.id_token) ||
        !read_string(fields, "refresh_token", stored.tokens.refresh_token) ||
        !read_integer(fields, "expires_in", expires_in) ||
        !read_integer(fields, "expires_at", expires_at) || expires_in < 0 ||
        expires_in > std::numeric_limits<int>::max())
    {
        std::cerr << "Ignoring malformed stored player tokens." << std::endl;
        return std::nullopt;
    }
    stored.tokens.expires_in = static_cast<int>(expires_in);
    stored.expires_at = static_cast<SDL_Time>(expires_at);
    return stored;
}

bool PlayerTokenStore::save(const StoredPlayerTokens &stored) const
{
    StorageCloser storage{open_ready()};
    if (storage.storage == nullptr) return false;

    const auto &tokens = stored.tokens;
    if (has_line_break(tokens.email) || has_line_break(tokens.access_token) ||
        has_line_break(tokens.id_token) || has_line_break(tokens.refresh_token))
    {
        std::cerr << "Token storage write error: tokens contain line breaks." << std::endl;
        return false;
    }
    const std::string contents = "email=" + tokens.email + "\naccess_token=" + tokens.access_token +
                                 "\nid_token=" + tokens.id_token +
                                 "\nrefresh_token=" + tokens.refresh_token +
                                 "\nexpires_in=" + std::to_string(tokens.expires_in) +
                                 "\nexpires_at=" + std::to_string(stored.expires_at) + "\n";
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

    for (const char *file : {TOKENS_FILE, LEGACY_TOKENS_FILE})
    {
        SDL_PathInfo info;
        if (!SDL_GetStoragePathInfo(storage.storage, file, &info)) continue;
        if (!SDL_RemoveStoragePath(storage.storage, file))
        {
            std::cerr << "Token storage remove error: " << SDL_GetError() << std::endl;
            return false;
        }
    }
    return true;
}
