#pragma once

#include "axf/core/export.hpp"
#include "axf/core/result.hpp"

#include <atomic>
#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>

namespace axf::core
{

/// Immutable published configuration. Reads never allocate and never lock.
class ConfigSnapshot final
{
  public:
    explicit ConfigSnapshot(std::map<std::string, std::string, std::less<>> values)
        : values_(std::move(values))
    {
    }

    [[nodiscard]] bool contains(std::string_view key) const noexcept;
    [[nodiscard]] std::size_t size() const noexcept
    {
        return values_.size();
    }

    /// Returns a value or `fallback`. Uses no allocation or locale facilities.
    [[nodiscard]] std::string_view getString(std::string_view key,
                                             std::string_view fallback) const noexcept;
    [[nodiscard]] int getInt(std::string_view key, int fallback) const noexcept;
    [[nodiscard]] float getFloat(std::string_view key, float fallback) const noexcept;
    [[nodiscard]] bool getBool(std::string_view key, bool fallback) const noexcept;

  private:
    std::map<std::string, std::string, std::less<>> values_;
};

/// Thread-safe control-thread store with an atomically published snapshot.
/// Audio threads use `withSnapshot()` and never call setters or file I/O.
class ConfigStore final
{
  public:
    ConfigStore();

    ConfigStore(const ConfigStore&) = delete;
    ConfigStore& operator=(const ConfigStore&) = delete;

    [[nodiscard]] Result<void> set(std::string key, std::string value);
    [[nodiscard]] Result<bool> remove(std::string_view key);
    [[nodiscard]] Result<std::string> get(std::string_view key) const;

    /// Publishes a new snapshot. Only call from a control thread.
    [[nodiscard]] Result<void> publish();

    [[nodiscard]] Result<void> loadFromFile(const std::string& path);
    [[nodiscard]] Result<void> saveToFile(const std::string& path) const;

    template <class Function> [[nodiscard]] decltype(auto) withSnapshot(Function&& function) const
    {
        auto snapshot = published_.load(std::memory_order_acquire);
        return function(*snapshot);
    }

    [[nodiscard]] std::size_t pendingChanges() const noexcept;

  private:
    mutable std::mutex mutex_;
    std::map<std::string, std::string, std::less<>> working_;
    std::atomic<std::shared_ptr<const ConfigSnapshot>> published_;
};

} // namespace axf::core
