#include <gtest/gtest.h>

#include "axf/audio/buffer.hpp"

using namespace axf;

TEST(AudioBuffer, DimensionsAndChannelIsolation)
{
    audio::AudioBuffer buffer(2, 8);
    EXPECT_EQ(buffer.channels(), 2);
    EXPECT_EQ(buffer.frames(), 8U);
    EXPECT_EQ(buffer.sampleCount(), 16U);
    for (std::size_t frame = 0; frame < buffer.frames(); ++frame)
    {
        EXPECT_FLOAT_EQ(buffer.channelData(0)[frame], 0.0F);
        EXPECT_FLOAT_EQ(buffer.channelData(1)[frame], 0.0F);
    }
    buffer.channelData(0)[3] = 0.25F;
    EXPECT_FLOAT_EQ(buffer.channelData(1)[3], 0.0F);
    EXPECT_EQ(buffer.channelData(2), nullptr);
}

TEST(AudioBuffer, FillClearAndCopy)
{
    audio::AudioBuffer source(2, 4);
    source.fill(0.5F);
    audio::AudioBuffer destination(2, 4);
    ASSERT_TRUE(destination.copyFrom(source).ok());
    EXPECT_FLOAT_EQ(destination.channelData(1)[3], 0.5F);
    destination.clear();
    EXPECT_FLOAT_EQ(destination.channelData(0)[0], 0.0F);

    audio::AudioBuffer mismatch(1, 4);
    const auto result = destination.copyFrom(mismatch);
    EXPECT_FALSE(result.ok());
    EXPECT_EQ(result.error().code, core::ErrorCode::InvalidArgument);
}

TEST(AudioBuffer, ViewPointsAtOriginalStorage)
{
    audio::AudioBuffer buffer(1, 2);
    auto view = audio::makeView(buffer);
    ASSERT_TRUE(view.valid());
    view.channel(0)[1] = 0.75F;
    EXPECT_FLOAT_EQ(buffer.channelData(0)[1], 0.75F);
}
