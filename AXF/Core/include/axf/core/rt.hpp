#pragma once

#include "axf/core/export.hpp"

#include <cstddef>
#include <cstdint>

namespace axf::core::rt
{

/// Marks the calling thread as an audio thread. Diagnostic only: the real-time
/// contract is enforced by code structure, not by this flag.
AXF_API void markAudioThread() noexcept;
AXF_API void clearAudioThread() noexcept;
[[nodiscard]] AXF_API bool isAudioThread() noexcept;

class ScopedAudioThread final
{
  public:
    ScopedAudioThread() noexcept;
    ~ScopedAudioThread() noexcept;
    ScopedAudioThread(const ScopedAudioThread&) = delete;
    ScopedAudioThread& operator=(const ScopedAudioThread&) = delete;

  private:
    bool previous_;
};

/// Called by allocation instrumentation. `deallocate == false` denotes an
/// allocation attempt; only those count as violations.
using AllocationHook = void (*)(std::size_t bytes, bool deallocate, void* userData) noexcept;

AXF_API void setAllocationHook(AllocationHook hook, void* userData) noexcept;
AXF_API void noteAllocation(std::size_t bytes, bool deallocate) noexcept;
AXF_API void noteRealtimeViolation() noexcept;
[[nodiscard]] AXF_API std::uint64_t violationCount() noexcept;
AXF_API void resetViolations() noexcept;

} // namespace axf::core::rt
