#pragma once

#include "axf/core/export.hpp"
#include "axf/core/result.hpp"

#include <cstdint>
#include <string_view>

namespace axf::core::thread
{

/// Sets the OS-visible name of the calling thread where supported. Safe on a
/// control thread; never call from an audio callback.
AXF_API void setCurrentThreadName(std::string_view name) noexcept;

/// Returns a process-local identifier for diagnostics.
[[nodiscard]] AXF_API std::uint64_t currentThreadId() noexcept;

enum class ThreadPriority : std::uint8_t
{
    Low,
    Normal,
    High
};

/// Requests a polite priority change. Real-time scheduling priorities are
/// deliberately not exposed; device threads are configured by their backend.
[[nodiscard]] AXF_API Result<void> setCurrentThreadPriority(ThreadPriority priority) noexcept;
[[nodiscard]] AXF_API const char* threadPriorityName(ThreadPriority priority) noexcept;

} // namespace axf::core::thread
