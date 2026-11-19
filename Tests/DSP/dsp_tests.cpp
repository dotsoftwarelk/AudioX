#include <gtest/gtest.h>

#include "axf/audio/buffer.hpp"
#include "axf/dsp/utility.hpp"

using namespace axf;

TEST(DspUtility, ApplyGainPreservesChannelIntegrity)
{
    audio::AudioBuffer buffer(2, 4);
    buffer.channelData(0)[0] = 1.0F;
    buffer.channelData(1)[0] = -1.0F;
    auto view = audio::makeView(buffer);
    dsp::applyGain(view, 0.5F);
    EXPECT_FLOAT_EQ(buffer.channelData(0)[0], 0.5F);
    EXPECT_FLOAT_EQ(buffer.channelData(1)[0], -0.5F);
}

TEST(DspUtility, UnityGainAndSilence)
{
    audio::AudioBuffer buffer(1, 4);
    buffer.fill(0.25F);
    auto view = audio::makeView(buffer);
    dsp::applyGain(view, 1.0F);
    EXPECT_FLOAT_EQ(buffer.channelData(0)[2], 0.25F);
    dsp::fillSilence(view);
    EXPECT_FLOAT_EQ(buffer.channelData(0)[2], 0.0F);
}

TEST(DspUtility, NonFiniteGainProducesSilence)
{
    audio::AudioBuffer buffer(1, 2);
    buffer.fill(1.0F);
    auto view = audio::makeView(buffer);
    dsp::applyGain(view, std::numeric_limits<float>::quiet_NaN());
    EXPECT_FLOAT_EQ(buffer.channelData(0)[0], 0.0F);
}

TEST(DspUtility, DenormalsAreFlushed)
{
    audio::AudioBuffer buffer(1, 2);
    buffer.channelData(0)[0] = 1.0e-40F;
    auto view = audio::makeView(buffer);
    dsp::applyGain(view, 1.0F);
    EXPECT_FLOAT_EQ(buffer.channelData(0)[0], 0.0F);
}
