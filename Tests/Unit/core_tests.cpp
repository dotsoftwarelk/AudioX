#include <gtest/gtest.h>

#include "axf/core/error.hpp"
#include "axf/core/log.hpp"
#include "axf/core/result.hpp"
#include "axf/core/spsc_ring_buffer.hpp"
#include "axf/core/types.hpp"

#include <string>
#include <vector>

using namespace axf::core;

TEST(CoreTypes, TimeConversions)
{
    EXPECT_DOUBLE_EQ(framesToSeconds(48000, 48000), 1.0);
    EXPECT_EQ(secondsToFrames(1.0, 48000), 48000);
    EXPECT_EQ(secondsToFrames(0.5, 44100), 22050);
    EXPECT_DOUBLE_EQ(framesToSeconds(100, 0), 0.0);
    EXPECT_EQ(secondsToFrames(1.0, 0), 0);
}

TEST(CoreTypes, GainConversions)
{
    EXPECT_FLOAT_EQ(dbToLinear(0.0F), 1.0F);
    EXPECT_NEAR(dbToLinear(-6.0F), 0.501187F, 1e-5F);
    EXPECT_TRUE(std::isinf(linearToDb(0.0F)));
    EXPECT_FLOAT_EQ(linearToDb(1.0F), 0.0F);
    EXPECT_FLOAT_EQ(dbToLinear(std::numeric_limits<float>::quiet_NaN()), 0.0F);
}

TEST(CoreTypes, PowerOfTwo)
{
    EXPECT_TRUE(isPowerOfTwo(1));
    EXPECT_TRUE(isPowerOfTwo(1024));
    EXPECT_FALSE(isPowerOfTwo(0));
    EXPECT_FALSE(isPowerOfTwo(1000));
}

TEST(CoreResult, SuccessAndFailure)
{
    Result<int> value{42};
    EXPECT_TRUE(value.ok());
    EXPECT_EQ(value.value(), 42);
    EXPECT_EQ(value.valueOr(7), 42);

    Result<int> failure{makeError(ErrorCode::NotFound, "missing")};
    EXPECT_FALSE(failure.ok());
    EXPECT_EQ(failure.error().code, ErrorCode::NotFound);
    EXPECT_EQ(failure.valueOr(7), 7);

    Result<void> voidSuccess = success();
    EXPECT_TRUE(voidSuccess.ok());
    Result<void> voidFailure{makeError(ErrorCode::IoError, "disk")};
    EXPECT_FALSE(voidFailure.ok());
    EXPECT_EQ(voidFailure.error().code, ErrorCode::IoError);
}

TEST(CoreError, CapturesLocationAndFormats)
{
    const Error error = makeError(ErrorCode::InvalidArgument, "bad argument");
    EXPECT_EQ(error.code, ErrorCode::InvalidArgument);
    EXPECT_STREQ(error.message, "bad argument");
    EXPECT_GT(error.line, 0);
    const std::string formatted = formatError(error);
    EXPECT_NE(formatted.find("invalid argument"), std::string::npos);
    EXPECT_NE(formatted.find("bad argument"), std::string::npos);
    EXPECT_STREQ(errorCodeName(ErrorCode::PluginError), "plugin error");
}

TEST(CoreSpsc, FifoOrderAndEmpty)
{
    SpscRingBuffer<int, 8> queue;
    EXPECT_TRUE(queue.empty());
    EXPECT_TRUE(queue.tryPush(1));
    EXPECT_TRUE(queue.tryPush(2));
    EXPECT_EQ(queue.sizeApprox(), 2U);
    int value = -1;
    EXPECT_TRUE(queue.tryPop(value));
    EXPECT_EQ(value, 1);
    EXPECT_TRUE(queue.tryPop(value));
    EXPECT_EQ(value, 2);
    EXPECT_FALSE(queue.tryPop(value));
}

TEST(CoreSpsc, FullAndWraparound)
{
    SpscRingBuffer<std::uint32_t, 4> queue;
    for (std::uint32_t i = 0; i < 4; ++i)
    {
        EXPECT_TRUE(queue.tryPush(i));
    }
    EXPECT_FALSE(queue.tryPush(99));
    for (std::uint32_t i = 0; i < 4; ++i)
    {
        std::uint32_t value = 0;
        ASSERT_TRUE(queue.tryPop(value));
        EXPECT_EQ(value, i);
    }
    for (std::uint32_t i = 10; i < 14; ++i)
    {
        EXPECT_TRUE(queue.tryPush(i));
    }
    for (std::uint32_t i = 10; i < 14; ++i)
    {
        std::uint32_t value = 0;
        ASSERT_TRUE(queue.tryPop(value));
        EXPECT_EQ(value, i);
    }
    EXPECT_TRUE(queue.empty());
}

namespace
{
class RecordingSink final : public ILogSink
{
  public:
    void write(const LogRecord& record) noexcept override
    {
        messages.emplace_back(record.message);
    }
    std::vector<std::string> messages;
};
} // namespace

TEST(CoreLog, CustomSinkReceivesFormattedMessage)
{
    RecordingSink sink;
    Logger::instance().setSink(&sink);
    AXF_LOG_INFO("value=", 7);
    Logger::instance().setSink(nullptr);
    ASSERT_EQ(sink.messages.size(), 1U);
    EXPECT_EQ(sink.messages.front(), "value=7");
    EXPECT_STREQ(logLevelName(LogLevel::Warning), "warn");
}
