#pragma once

#include "axf/core/export.hpp"

#include <cstdint>
#include <string_view>

namespace axf::core::thread
{

/// Sets the OS-visible name of the calling thread where supported. Safe on a
/// control thread; never call from an audio callback.
AXF_API void setCurrentThreadName(std::string_view name) noexcept;

/// Returns a process-local identifier for diagnostics.
[[nodiscard]] AXF_API std::uint64_t currentThreadId() noexcept;

} // namespace axf::core::thread
