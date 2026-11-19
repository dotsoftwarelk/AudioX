#include <gtest/gtest.h>

#include "axf/dsp/panner.hpp"

#include <array>
#include <cmath>

using namespace axf;

namespace
{
struct StereoProcess
{
    std::array<float, 4> input{1.0F, 1.0F, 1.0F, 1.0F};
    std::array<float, 4> left{};
    std::array<float, 4> right{};
    std::array<float*, 1> sourcePointers{input.data()};
    std::array<float*, 1> leftPointers{left.data()};
    std::array<float*, 1> rightPointers{right.data()};
    audio::AudioBufferView source{sourcePointers.data(), 1, input.size()};
    audio::AudioBufferView outLeft{leftPointers.data(), 1, left.size()};
    audio::AudioBufferView outRight{rightPointers.data(), 1, right.size()};
    audio::AudioBufferView outputs[2]{outLeft, outRight};
    audio::ProcessContext context{&source, 1, outputs, 2, 4, 48000.0, 0};
};

double sumOfSquares(float left, float right) noexcept
{
    return static_cast<double>(left) * left + static_cast<double>(right) * right;
}
} // namespace

TEST(Panner, CentreIsConstantPower)
{
    dsp::PannerProcessor panner;
    StereoProcess audio;
    panner.process(audio.context);
    EXPECT_NEAR(audio.left[0], std::sqrt(0.5F), 1.0e-5);
    EXPECT_NEAR(audio.right[0], std::sqrt(0.5F), 1.0e-5);
    EXPECT_NEAR(sumOfSquares(audio.left[0], audio.right[0]), 1.0, 1.0e-5);
}

TEST(Panner, HardPansAndAtomicChanges)
{
    dsp::PannerProcessor panner(-1.0F);
    StereoProcess audio;
    panner.process(audio.context);
    EXPECT_FLOAT_EQ(audio.left[0], 1.0F);
    EXPECT_FLOAT_EQ(audio.right[0], 0.0F);

    panner.setPan(1.0F);
    StereoProcess other;
    panner.process(other.context);
    EXPECT_NEAR(other.left[0], 0.0F, 1.0e-6F);
    EXPECT_FLOAT_EQ(other.right[0], 1.0F);

    panner.setPan(std::numeric_limits<float>::quiet_NaN());
    EXPECT_FLOAT_EQ(panner.pan(), 0.0F);
    panner.reset();
    EXPECT_FLOAT_EQ(panner.pan(), 0.0F);
}
