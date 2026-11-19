#include "axf/platform/platform.hpp"

namespace axf::platform
{

const char* osName() noexcept
{
    switch (currentOs())
    {
    case OsKind::Windows:
        return "Windows";
    case OsKind::Linux:
        return "Linux";
    case OsKind::MacOS:
        return "macOS";
    case OsKind::Unknown:
        return "Unknown";
    }
    return "Unknown";
}

const char* frameworkVersion() noexcept
{
    return "0.1.0-phase0";
}

} // namespace axf::platform
