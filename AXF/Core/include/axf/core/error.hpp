#pragma once

#include "axf/core/export.hpp"

#include <cstdint>
#include <source_location>
#include <string>

namespace axf::core
{

enum class ErrorCode : std::uint8_t
{
    None = 0,
    InvalidArgument,
    OutOfRange,
    NotFound,
    AlreadyExists,
    Unsupported,
    OutOfMemory,
    IoError,
    DeviceError,
    PluginError,
    MidiError,
    ThreadError,
    TimedOut,
    Cancelled,
    InternalError,
};

/// Lightweight, allocation-free error value. `message` must point to static
/// storage (string literal) so errors can be created on the audio thread.
struct Error final
{
    ErrorCode code{ErrorCode::None};
    const char* message{"ok"};
    const char* file{""};
    int line{0};

    [[nodiscard]] explicit operator bool() const noexcept
    {
        return code != ErrorCode::None;
    }

    friend bool operator==(const Error&, const Error&) noexcept = default;
};

[[nodiscard]] AXF_API Error
makeError(ErrorCode code, const char* message,
          std::source_location location = std::source_location::current()) noexcept;

[[nodiscard]] AXF_API const char* errorCodeName(ErrorCode code) noexcept;

/// Allocating, human-readable form. Never call from the audio callback.
[[nodiscard]] AXF_API std::string formatError(const Error& error);

} // namespace axf::core

#define AXF_ERROR(code, text) ::axf::core::makeError((code), (text))
