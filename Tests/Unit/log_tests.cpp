#include <gtest/gtest.h>

#include "axf/core/log.hpp"
#include "axf/core/rt.hpp"

#include <string>
#include <vector>

using namespace axf::core;

namespace
{
class CollectingSink final : public ILogSink
{
  public:
    void write(const LogRecord& record) noexcept override
    {
        messages.emplace_back(record.message);
    }
    std::vector<std::string> messages;
};
} // namespace

TEST(Logger, MultipleSinksReceiveRecords)
{
    CollectingSink first;
    CollectingSink second;
    Logger& logger = Logger::instance();
    logger.clearSinks();
    logger.addSink(&first);
    logger.addSink(&second);
    AXF_LOG_WARN("value=", 12);
    logger.clearSinks();
    logger.setMinLevel(LogLevel::Trace);

    EXPECT_EQ(first.messages.size(), 1U);
    EXPECT_EQ(second.messages.size(), 1U);
    EXPECT_EQ(first.messages.front(), "value=12");
}

TEST(Logger, MinimumLevelFiltersRecords)
{
    CollectingSink sink;
    Logger& logger = Logger::instance();
    logger.setSink(&sink);
    logger.setMinLevel(LogLevel::Warning);
    AXF_LOG_INFO("hidden");
    AXF_LOG_ERROR("shown");
    logger.setSink(nullptr);
    logger.setMinLevel(LogLevel::Trace);

    ASSERT_EQ(sink.messages.size(), 1U);
    EXPECT_EQ(sink.messages.front(), "shown");
}

TEST(Logger, AudioThreadLoggingIsCountedNotPerformed)
{
    CollectingSink sink;
    Logger& logger = Logger::instance();
    logger.setSink(&sink);
    rt::resetViolations();
    {
        rt::ScopedAudioThread guard;
        AXF_LOG_INFO("would block");
    }
    logger.setSink(nullptr);
    EXPECT_EQ(rt::violationCount(), 1U);
    EXPECT_TRUE(sink.messages.empty());
}
