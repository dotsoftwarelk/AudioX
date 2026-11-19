#pragma once

#include "axf/core/export.hpp"

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>

namespace axf::core
{

enum class JobPriority : std::uint8_t
{
    Low = 0,
    Normal = 1,
    High = 2
};

/// Fixed worker pool for control-thread background work. The audio callback
/// must never submit to, or wait on, this system.
class JobSystem final
{
  public:
    explicit JobSystem(std::uint32_t workerCount = 0);
    ~JobSystem();

    JobSystem(const JobSystem&) = delete;
    JobSystem& operator=(const JobSystem&) = delete;

    void submit(std::function<void()> job, JobPriority priority = JobPriority::Normal);

    /// Blocks until all submitted jobs have finished. Control thread only.
    void waitForAll();

    /// Stops workers after draining queued jobs. Idempotent.
    void shutdown();

    [[nodiscard]] std::uint32_t workerCount() const noexcept;
    [[nodiscard]] std::size_t pendingCount() const noexcept;
    [[nodiscard]] bool isShutdown() const noexcept;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace axf::core
