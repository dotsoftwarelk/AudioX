#include <gtest/gtest.h>

#include "axf/dsp/biquad.hpp"

#include <array>
#include <cmath>
#include <vector>

using namespace axf;

namespace
{
constexpr double kSampleRate = 48000.0;
constexpr std::size_t kWarmup = 1024;
constexpr std::size_t kWindow = 8192;

double goertzelMagnitude(const std::vector<float>& signal, double frequency) noexcept
{
    const double coefficient =
        2.0 * std::cos(2.0 * 3.14159265358979323846 * frequency / kSampleRate);
    double previous = 0.0;
    double previous2 = 0.0;
    for (const float sample : signal)
    {
        const double current = static_cast<double>(sample) + coefficient * previous - previous2;
        previous2 = previous;
        previous = current;
    }
    const double power =
        previous * previous + previous2 * previous2 - coefficient * previous * previous2;
    return 2.0 * std::sqrt(std::max(0.0, power)) / static_cast<double>(signal.size());
}

double responseAt(dsp::BiquadType type, double frequency, double gainDb = 0.0)
{
    dsp::BiquadDesign design;
    design.type = type;
    design.frequencyHz = 1000.0;
    design.q = 0.7071067811865476;
    design.gainDb = gainDb;
    const auto coefficients = dsp::designBiquad(design, kSampleRate);
    EXPECT_TRUE(coefficients.ok());
    if (!coefficients.ok())
    {
        return 0.0;
    }

    dsp::BiquadProcessor processor(coefficients.value());
    std::vector<float> output;
    output.reserve(kWindow);
    double phase = 0.0;
    for (std::size_t index = 0; index < kWarmup + kWindow; ++index)
    {
        const float input = static_cast<float>(std::sin(2.0 * 3.14159265358979323846 * frequency *
                                                        static_cast<double>(index) / kSampleRate));
        std::array<float, 1> buffer{input};
        float* pointers[]{buffer.data()};
        audio::AudioBufferView view{pointers, 1, 1};
        processor.process(view);
        if (index >= kWarmup)
        {
            output.push_back(buffer[0]);
        }
        (void)phase;
    }
    return goertzelMagnitude(output, frequency);
}
} // namespace

TEST(BiquadDesign, ValidatesParameters)
{
    dsp::BiquadDesign design;
    EXPECT_TRUE(dsp::designBiquad(design, kSampleRate).ok());

    design.frequencyHz = 24000.0;
    EXPECT_EQ(dsp::designBiquad(design, kSampleRate).error().code, core::ErrorCode::OutOfRange);

    design = dsp::BiquadDesign{};
    EXPECT_EQ(dsp::designBiquad(design, 0.0).error().code, core::ErrorCode::OutOfRange);

    design = dsp::BiquadDesign{};
    design.q = 0.0;
    EXPECT_EQ(dsp::designBiquad(design, kSampleRate).error().code, core::ErrorCode::OutOfRange);

    design = dsp::BiquadDesign{};
    design.gainDb = std::numeric_limits<double>::quiet_NaN();
    EXPECT_EQ(dsp::designBiquad(design, kSampleRate).error().code,
              core::ErrorCode::InvalidArgument);
}

TEST(BiquadResponse, LowPassAttenuatesHighFrequencies)
{
    EXPECT_GT(responseAt(dsp::BiquadType::LowPass, 200.0), 0.7);
    EXPECT_LT(responseAt(dsp::BiquadType::LowPass, 8000.0), 0.2);
}

TEST(BiquadResponse, HighPassAttenuatesLowFrequencies)
{
    EXPECT_LT(responseAt(dsp::BiquadType::HighPass, 200.0), 0.2);
    EXPECT_GT(responseAt(dsp::BiquadType::HighPass, 8000.0), 0.7);
}

TEST(BiquadResponse, PeakingBoostsCentreOnly)
{
    EXPECT_GT(responseAt(dsp::BiquadType::Peaking, 1000.0, 6.0), 1.4);
    const double away = responseAt(dsp::BiquadType::Peaking, 100.0, 6.0);
    EXPECT_GT(away, 0.8);
    EXPECT_LT(away, 1.2);
}

TEST(BiquadState, ResetClearsTailAndProcessContextMatchesBuffer)
{
    dsp::BiquadDesign design;
    design.frequencyHz = 500.0;
    const auto coefficients = dsp::designBiquad(design, kSampleRate);
    ASSERT_TRUE(coefficients.ok());
    dsp::BiquadProcessor processor(coefficients.value());

    std::array<float, 8> input{};
    input.fill(1.0F);
    float* inputPointers[]{input.data()};
    audio::AudioBufferView view{inputPointers, 1, input.size()};
    for (int block = 0; block < 20; ++block)
    {
        processor.process(view);
    }
    ASSERT_NE(input[0], 0.0F);

    processor.reset();
    std::array<float, 8> silence{};
    float* silentPointers[]{silence.data()};
    audio::AudioBufferView silentView{silentPointers, 1, silence.size()};
    processor.process(silentView);
    for (const float sample : silence)
    {
        EXPECT_FLOAT_EQ(sample, 0.0F);
    }

    std::array<float, 4> frames{1.0F, 0.0F, 0.0F, 0.0F};
    std::array<float, 4> destination{};
    float* sourcePointers[]{frames.data()};
    float* destinationPointers[]{destination.data()};
    audio::AudioBufferView sourceView{sourcePointers, 1, frames.size()};
    audio::AudioBufferView destinationView{destinationPointers, 1, destination.size()};
    audio::ProcessContext context;
    context.inputs = &sourceView;
    context.inputCount = 1;
    context.outputs = &destinationView;
    context.outputCount = 1;
    context.frameCount = static_cast<std::uint32_t>(frames.size());
    processor.process(context);
    EXPECT_FLOAT_EQ(destination[0], static_cast<float>(coefficients.value().b0));
}
