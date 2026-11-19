#include "axf/core/config.hpp"

#include <charconv>
#include <fstream>
#include <utility>

namespace axf::core
{
namespace
{
using ConfigMap = std::map<std::string, std::string, std::less<>>;

std::string_view trim(std::string_view value) noexcept
{
    while (!value.empty() && (value.front() == ' ' || value.front() == '\t' ||
                              value.front() == '\r' || value.front() == '\n'))
    {
        value.remove_prefix(1);
    }
    while (!value.empty() && (value.back() == ' ' || value.back() == '\t' || value.back() == '\r' ||
                              value.back() == '\n'))
    {
        value.remove_suffix(1);
    }
    return value;
}

bool parseLine(const std::string& line, std::string& key, std::string& value)
{
    const std::string_view trimmed = trim(line);
    if (trimmed.empty() || trimmed.front() == '#')
    {
        return true;
    }
    const std::size_t equals = trimmed.find('=');
    if (equals == std::string_view::npos)
    {
        return false;
    }
    key.assign(trim(trimmed.substr(0, equals)));
    value.assign(trim(trimmed.substr(equals + 1)));
    return !key.empty();
}
} // namespace

bool ConfigSnapshot::contains(std::string_view key) const noexcept
{
    return values_.find(key) != values_.end();
}

std::string_view ConfigSnapshot::getString(std::string_view key,
                                           std::string_view fallback) const noexcept
{
    const auto it = values_.find(key);
    return it == values_.end() ? fallback : std::string_view(it->second);
}

int ConfigSnapshot::getInt(std::string_view key, int fallback) const noexcept
{
    const auto it = values_.find(key);
    if (it == values_.end())
    {
        return fallback;
    }
    int result = fallback;
    const auto [end, error] =
        std::from_chars(it->second.data(), it->second.data() + it->second.size(), result);
    return error == std::errc{} ? result : fallback;
}

float ConfigSnapshot::getFloat(std::string_view key, float fallback) const noexcept
{
    const auto it = values_.find(key);
    if (it == values_.end())
    {
        return fallback;
    }
    float result = fallback;
    const auto [end, error] =
        std::from_chars(it->second.data(), it->second.data() + it->second.size(), result);
    return error == std::errc{} ? result : fallback;
}

bool ConfigSnapshot::getBool(std::string_view key, bool fallback) const noexcept
{
    const auto it = values_.find(key);
    if (it == values_.end() || it->second.empty())
    {
        return fallback;
    }
    switch (it->second.front())
    {
    case '1':
    case 't':
    case 'T':
    case 'y':
    case 'Y':
        return true;
    case '0':
    case 'f':
    case 'F':
    case 'n':
    case 'N':
        return false;
    default:
        return fallback;
    }
}

ConfigStore::ConfigStore() : published_(std::make_shared<const ConfigSnapshot>(ConfigMap{}))
{
}

Result<void> ConfigStore::set(std::string key, std::string value)
{
    if (key.empty() || key.find('=') != std::string::npos)
    {
        return makeError(ErrorCode::InvalidArgument, "invalid configuration key");
    }
    {
        std::lock_guard<std::mutex> lock(mutex_);
        working_.insert_or_assign(std::move(key), std::move(value));
    }
    return publish();
}

Result<bool> ConfigStore::remove(std::string_view key)
{
    bool removed = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto it = working_.find(key);
        if (it != working_.end())
        {
            working_.erase(it);
            removed = true;
        }
    }
    if (removed)
    {
        const auto result = publish();
        if (!result)
        {
            return result.error();
        }
    }
    return removed;
}

Result<std::string> ConfigStore::get(std::string_view key) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    const auto it = working_.find(key);
    if (it == working_.end())
    {
        return makeError(ErrorCode::NotFound, "configuration key not found");
    }
    return it->second;
}

Result<void> ConfigStore::publish()
{
    ConfigMap copy;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        copy = working_;
    }
    auto snapshot = std::make_shared<const ConfigSnapshot>(std::move(copy));
    published_.store(snapshot, std::memory_order_release);
    return success();
}

Result<void> ConfigStore::loadFromFile(const std::string& path)
{
    std::ifstream stream(path);
    if (!stream.is_open())
    {
        return makeError(ErrorCode::IoError, "cannot open configuration file");
    }

    ConfigMap loaded;
    std::string line;
    while (std::getline(stream, line))
    {
        std::string key;
        std::string value;
        if (!parseLine(line, key, value))
        {
            return makeError(ErrorCode::InvalidArgument, "malformed configuration line");
        }
        if (!key.empty())
        {
            loaded.insert_or_assign(std::move(key), std::move(value));
        }
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        working_ = std::move(loaded);
    }
    return publish();
}

Result<void> ConfigStore::saveToFile(const std::string& path) const
{
    ConfigMap copy;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        copy = working_;
    }

    std::ofstream stream(path, std::ios::trunc);
    if (!stream.is_open())
    {
        return makeError(ErrorCode::IoError, "cannot open configuration file for writing");
    }
    for (const auto& [key, value] : copy)
    {
        stream << key << '=' << value << '\n';
    }
    if (!stream.good())
    {
        return makeError(ErrorCode::IoError, "failed to write configuration file");
    }
    return success();
}

std::size_t ConfigStore::pendingChanges() const noexcept
{
    std::lock_guard<std::mutex> lock(mutex_);
    const auto snapshot = published_.load(std::memory_order_acquire);
    return working_.size() > snapshot->size() ? working_.size() - snapshot->size() : 0U;
}

} // namespace axf::core
