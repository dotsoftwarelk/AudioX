#pragma once

#include "axf/core/export.hpp"

#include <cstdint>

namespace axf::platform
{

enum class OsKind : std::uint8_t
{
    Windows,
    Linux,
    MacOS,
    Unknown
};

[[nodiscard]] constexpr OsKind currentOs() noexcept
{
#if defined(_WIN32)
    return OsKind::Windows;
#elif defined(__APPLE__)
    return OsKind::MacOS;
#elif defined(__linux__)
    return OsKind::Linux;
#else
    return OsKind::Unknown;
#endif
}

[[nodiscard]] AXF_API const char* osName() noexcept;
[[nodiscard]] AXF_API const char* frameworkVersion() noexcept;

} // namespace axf::platform
