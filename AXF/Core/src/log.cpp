#include "axf/core/log.hpp"
#include "axf/core/rt.hpp"

#include <algorithm>
#include <iostream>
#include <mutex>
#include <vector>

namespace axf::core
{
namespace
{

class ConsoleSink final : public ILogSink
{
  public:
    void write(const LogRecord& record) noexcept override
    {
        std::cerr << '[' << logLevelName(record.level) << "] " << record.message;
        if (!record.file.empty())
        {
            std::cerr << " (" << record.file << ':' << record.line << ')';
        }
        std::cerr << '\n';
    }
};

} // namespace

struct Logger::Impl final
{
    mutable std::mutex mutex;
    ConsoleSink consoleSink;
    std::vector<ILogSink*> sinks;
    LogLevel minimum{LogLevel::Trace};
};

Logger::Logger() : impl_(std::make_unique<Impl>())
{
    impl_->sinks.push_back(&impl_->consoleSink);
}

Logger::~Logger() = default;

Logger& Logger::instance() noexcept
{
    static Logger instance;
    return instance;
}

void Logger::setSink(ILogSink* sink) noexcept
{
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->sinks.clear();
    impl_->sinks.push_back(sink == nullptr ? &impl_->consoleSink : sink);
}

void Logger::addSink(ILogSink* sink) noexcept
{
    if (sink == nullptr)
    {
        return;
    }
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (std::find(impl_->sinks.begin(), impl_->sinks.end(), sink) == impl_->sinks.end())
    {
        impl_->sinks.push_back(sink);
    }
}

void Logger::removeSink(ILogSink* sink) noexcept
{
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->sinks.erase(std::remove(impl_->sinks.begin(), impl_->sinks.end(), sink),
                       impl_->sinks.end());
    if (impl_->sinks.empty())
    {
        impl_->sinks.push_back(&impl_->consoleSink);
    }
}

void Logger::clearSinks() noexcept
{
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->sinks.clear();
    impl_->sinks.push_back(&impl_->consoleSink);
}

void Logger::setMinLevel(LogLevel level) noexcept
{
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->minimum = level;
}

LogLevel Logger::minLevel() const noexcept
{
    std::lock_guard<std::mutex> lock(impl_->mutex);
    return impl_->minimum;
}

void Logger::log(LogLevel level, std::string_view message, std::source_location location)
{
    if (rt::isAudioThread())
    {
        rt::noteRealtimeViolation();
        return;
    }

    LogRecord record{level, message, location.file_name(), static_cast<int>(location.line())};
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (static_cast<std::uint8_t>(level) < static_cast<std::uint8_t>(impl_->minimum))
    {
        return;
    }
    for (ILogSink* sink : impl_->sinks)
    {
        sink->write(record);
    }
}

const char* logLevelName(LogLevel level) noexcept
{
    switch (level)
    {
    case LogLevel::Trace:
        return "trace";
    case LogLevel::Debug:
        return "debug";
    case LogLevel::Info:
        return "info";
    case LogLevel::Warning:
        return "warn";
    case LogLevel::Error:
        return "error";
    case LogLevel::Critical:
        return "critical";
    }
    return "unknown";
}

} // namespace axf::core
