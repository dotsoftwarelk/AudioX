#pragma once

#include "axf/core/export.hpp"

#include <cstdint>
#include <memory>
#include <source_location>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

namespace axf::core
{

enum class LogLevel : std::uint8_t
{
    Trace = 0,
    Debug,
    Info,
    Warning,
    Error,
    Critical,
};

struct LogRecord final
{
    LogLevel level{LogLevel::Info};
    std::string_view message;
    std::string_view file;
    int line{0};
};

/// Sinks complete synchronously. The default logger takes a mutex, so it must
/// never be called from the audio callback.
class ILogSink
{
  public:
    virtual ~ILogSink() = default;
    virtual void write(const LogRecord& record) noexcept = 0;
};

class Logger final
{
  public:
    static Logger& instance() noexcept;

    /// Replaces all sinks with a single sink (nullptr restores the console).
    void setSink(ILogSink* sink) noexcept;
    void addSink(ILogSink* sink) noexcept;
    void removeSink(ILogSink* sink) noexcept;
    void clearSinks() noexcept;

    void setMinLevel(LogLevel level) noexcept;
    [[nodiscard]] LogLevel minLevel() const noexcept;

    /// Logging from an audio thread is a contract violation: it is dropped and
    /// counted by the real-time audit instead of taking the logger mutex.
    void log(LogLevel level, std::string_view message,
             std::source_location location = std::source_location::current());

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

  private:
    Logger();
    ~Logger();

    struct Impl;
    std::unique_ptr<Impl> impl_;
};

[[nodiscard]] AXF_API const char* logLevelName(LogLevel level) noexcept;

namespace detail
{
inline void appendFields(std::ostringstream&)
{
}

template <class T, class... Rest>
void appendFields(std::ostringstream& stream, T&& first, Rest&&... rest)
{
    stream << std::forward<T>(first);
    appendFields(stream, std::forward<Rest>(rest)...);
}
} // namespace detail

template <class... Args>
void writeLog(LogLevel level, std::source_location location, Args&&... args)
{
    std::ostringstream stream;
    detail::appendFields(stream, std::forward<Args>(args)...);
    Logger::instance().log(level, stream.str(), location);
}

} // namespace axf::core

#define AXF_LOG_TRACE(...)                                                                         \
    ::axf::core::writeLog(::axf::core::LogLevel::Trace, std::source_location::current(),           \
                          __VA_ARGS__)
#define AXF_LOG_DEBUG(...)                                                                         \
    ::axf::core::writeLog(::axf::core::LogLevel::Debug, std::source_location::current(),           \
                          __VA_ARGS__)
#define AXF_LOG_INFO(...)                                                                          \
    ::axf::core::writeLog(::axf::core::LogLevel::Info, std::source_location::current(), __VA_ARGS__)
#define AXF_LOG_WARN(...)                                                                          \
    ::axf::core::writeLog(::axf::core::LogLevel::Warning, std::source_location::current(),         \
                          __VA_ARGS__)
#define AXF_LOG_ERROR(...)                                                                         \
    ::axf::core::writeLog(::axf::core::LogLevel::Error, std::source_location::current(),           \
                          __VA_ARGS__)
