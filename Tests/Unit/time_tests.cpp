#include <gtest/gtest.h>

#include "axf/core/time.hpp"

using namespace axf::core;

TEST(TimeConversions, BeatsAndSeconds)
{
    EXPECT_DOUBLE_EQ(time::beatsToSeconds(4.0, 120.0), 2.0);
    EXPECT_DOUBLE_EQ(time::secondsToBeats(2.0, 120.0), 4.0);
    EXPECT_DOUBLE_EQ(time::beatsToSeconds(4.0, 0.0), 0.0);
    EXPECT_DOUBLE_EQ(time::secondsToBeats(2.0, -1.0), 0.0);
}

TEST(TimeConversions, BeatsAndFrames)
{
    EXPECT_EQ(time::beatsToFrames(4.0, 120.0, 48000), 96000);
    EXPECT_DOUBLE_EQ(time::framesToBeats(96000, 120.0, 48000), 4.0);
}

TEST(TimeConversions, Ticks)
{
    EXPECT_EQ(time::beatsToTicks(2.0, 960.0), 1920U);
    EXPECT_DOUBLE_EQ(time::ticksToBeats(1920U, 960.0), 2.0);
    EXPECT_EQ(time::beatsToTicks(2.0, 0.0), 0U);
}

TEST(TimeConversions, TimecodeFormatting)
{
    EXPECT_EQ(time::formatTimecode(48000, 48000), "00:00:01:00");
    EXPECT_EQ(time::formatTimecode(0, 48000), "00:00:00:00");
    EXPECT_EQ(time::formatTimecode(-1, 48000), "--:--:--:--");
}
