#include "axf/core/log.hpp"

#include <iostream>
#include <mutex>

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
    std::mutex mutex;
    ConsoleSink consoleSink;
    ILogSink* sink{&consoleSink};
};

Logger::Logger() : impl_(std::make_unique<Impl>())
{
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
    impl_->sink = sink == nullptr ? &impl_->consoleSink : sink;
}

void Logger::log(LogLevel level, std::string_view message, std::source_location location)
{
    const LogRecord record{level, message, location.file_name(), static_cast<int>(location.line())};
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->sink->write(record);
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
