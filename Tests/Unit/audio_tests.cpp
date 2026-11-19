#include <gtest/gtest.h>

#include "axf/audio/clock.hpp"
#include "axf/audio/transport.hpp"

using namespace axf;

TEST(AudioClock, AdvancesAndConverts)
{
    audio::AudioClock clock{48000};
    EXPECT_EQ(clock.currentFrame(), 0);
    clock.advance(48000);
    EXPECT_EQ(clock.currentFrame(), 48000);
    EXPECT_DOUBLE_EQ(clock.currentSeconds(), 1.0);
    clock.setFrame(-5);
    EXPECT_EQ(clock.currentFrame(), 0);
    clock.setSampleRate(44100);
    EXPECT_EQ(clock.sampleRate(), 44100);
}

TEST(AudioTransport, RollingAndPosition)
{
    audio::Transport transport{48000};
    EXPECT_EQ(transport.state(), audio::Transport::State::Stopped);
    EXPECT_FALSE(transport.isRolling());
    transport.play();
    EXPECT_TRUE(transport.isRolling());
    audio::advanceTransport(transport, 512);
    EXPECT_EQ(transport.position(), 512);

    transport.pause();
    audio::advanceTransport(transport, 512);
    EXPECT_EQ(transport.position(), 512);

    transport.record();
    audio::advanceTransport(transport, 128);
    EXPECT_EQ(transport.position(), 640);

    transport.stop();
    EXPECT_FALSE(transport.isRolling());
    EXPECT_STREQ(audio::transportStateName(transport.state()), "stopped");
}

TEST(AudioTransport, RejectsNegativePosition)
{
    audio::Transport transport{48000};
    transport.setPosition(-10);
    EXPECT_EQ(transport.position(), 0);
    audio::advanceTransport(transport, -4);
    EXPECT_EQ(transport.position(), 0);
}
