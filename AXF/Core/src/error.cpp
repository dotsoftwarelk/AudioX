#include "axf/core/error.hpp"

namespace axf::core
{

Error makeError(ErrorCode code, const char* message, std::source_location location) noexcept
{
    return Error{code, message == nullptr ? "unknown error" : message, location.file_name(),
                 static_cast<int>(location.line())};
}

const char* errorCodeName(ErrorCode code) noexcept
{
    switch (code)
    {
    case ErrorCode::None:
        return "none";
    case ErrorCode::InvalidArgument:
        return "invalid argument";
    case ErrorCode::OutOfRange:
        return "out of range";
    case ErrorCode::NotFound:
        return "not found";
    case ErrorCode::AlreadyExists:
        return "already exists";
    case ErrorCode::Unsupported:
        return "unsupported";
    case ErrorCode::OutOfMemory:
        return "out of memory";
    case ErrorCode::IoError:
        return "I/O error";
    case ErrorCode::DeviceError:
        return "device error";
    case ErrorCode::PluginError:
        return "plugin error";
    case ErrorCode::MidiError:
        return "MIDI error";
    case ErrorCode::ThreadError:
        return "thread error";
    case ErrorCode::TimedOut:
        return "timed out";
    case ErrorCode::Cancelled:
        return "cancelled";
    case ErrorCode::InternalError:
        return "internal error";
    }
    return "unknown";
}

std::string formatError(const Error& error)
{
    std::string result;
    result.reserve(128);
    result += errorCodeName(error.code);
    result += ": ";
    result += error.message == nullptr ? "unknown error" : error.message;
    if (error.line > 0 && error.file != nullptr && error.file[0] != '\0')
    {
        result += " (";
        result += error.file;
        result += ':';
        result += std::to_string(error.line);
        result += ')';
    }
    return result;
}

} // namespace axf::core
