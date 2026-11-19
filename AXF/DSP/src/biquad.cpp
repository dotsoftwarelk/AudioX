#include "axf/dsp/biquad.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace axf::dsp
{
namespace
{
constexpr double kPi = 3.14159265358979323846;
constexpr double kDenormalThreshold = 1.0e-30;

double flush(double value) noexcept
{
    return std::abs(value) < kDenormalThreshold ? 0.0 : value;
}
} // namespace

core::Result<BiquadCoefficients> designBiquad(const BiquadDesign& design,
                                              double sampleRate) noexcept
{
    if (!std::isfinite(sampleRate) || sampleRate <= 0.0)
    {
        return core::makeError(core::ErrorCode::OutOfRange, "sample rate must be positive");
    }
    if (!std::isfinite(design.frequencyHz) || design.frequencyHz <= 0.0 ||
        design.frequencyHz >= sampleRate * 0.5)
    {
        return core::makeError(core::ErrorCode::OutOfRange,
                               "frequency must lie strictly below the Nyquist limit");
    }
    if (!std::isfinite(design.q) || design.q <= 0.0)
    {
        return core::makeError(core::ErrorCode::OutOfRange, "Q must be positive and finite");
    }
    if (!std::isfinite(design.gainDb))
    {
        return core::makeError(core::ErrorCode::InvalidArgument, "gain must be finite");
    }

    const double omega = 2.0 * kPi * design.frequencyHz / sampleRate;
    const double cosine = std::cos(omega);
    const double sine = std::sin(omega);
    const double alpha = sine / (2.0 * design.q);

    double b0 = 0.0;
    double b1 = 0.0;
    double b2 = 0.0;
    double a0 = 1.0;
    double a1 = 0.0;
    double a2 = 0.0;

    switch (design.type)
    {
    case BiquadType::LowPass:
    {
        b0 = (1.0 - cosine) * 0.5;
        b1 = 1.0 - cosine;
        b2 = b0;
        a0 = 1.0 + alpha;
        a1 = -2.0 * cosine;
        a2 = 1.0 - alpha;
        break;
    }
    case BiquadType::HighPass:
    {
        b0 = (1.0 + cosine) * 0.5;
        b1 = -(1.0 + cosine);
        b2 = b0;
        a0 = 1.0 + alpha;
        a1 = -2.0 * cosine;
        a2 = 1.0 - alpha;
        break;
    }
    case BiquadType::Peaking:
    {
        const double amplitude = std::pow(10.0, design.gainDb / 40.0);
        b0 = 1.0 + alpha * amplitude;
        b1 = -2.0 * cosine;
        b2 = 1.0 - alpha * amplitude;
        a0 = 1.0 + alpha / amplitude;
        a1 = -2.0 * cosine;
        a2 = 1.0 - alpha / amplitude;
        break;
    }
    }

    if (!std::isfinite(a0) || std::abs(a0) < std::numeric_limits<double>::min())
    {
        return core::makeError(core::ErrorCode::InternalError, "invalid normalisation coefficient");
    }

    BiquadCoefficients result;
    result.b0 = b0 / a0;
    result.b1 = b1 / a0;
    result.b2 = b2 / a0;
    result.a1 = a1 / a0;
    result.a2 = a2 / a0;
    return result;
}

BiquadProcessor::BiquadProcessor(BiquadCoefficients coefficients) noexcept
    : coefficients_(coefficients)
{
}

void BiquadProcessor::prepare(double, std::uint32_t) noexcept
{
}

void BiquadProcessor::reset() noexcept
{
    for (State& state : states_)
    {
        state = State{};
    }
}

void BiquadProcessor::processChannel(const float* source, float* destination, std::size_t frames,
                                     State& state) const noexcept
{
    if (source == nullptr || destination == nullptr)
    {
        return;
    }
    for (std::size_t frame = 0; frame < frames; ++frame)
    {
        const double input = static_cast<double>(source[frame]);
        const double output = coefficients_.b0 * input + coefficients_.b1 * state.x1 +
                              coefficients_.b2 * state.x2 - coefficients_.a1 * state.y1 -
                              coefficients_.a2 * state.y2;
        state.x2 = state.x1;
        state.x1 = input;
        state.y2 = state.y1;
        state.y1 = flush(output);
        destination[frame] = static_cast<float>(state.y1);
    }
}

void BiquadProcessor::process(audio::ProcessContext& context) noexcept
{
    if (context.inputCount == 0 || context.outputCount == 0 || context.inputs == nullptr ||
        context.outputs == nullptr)
    {
        return;
    }
    const audio::AudioBufferView& source = context.inputs[0];
    audio::AudioBufferView& destination = context.outputs[0];
    const auto channels =
        static_cast<std::uint16_t>(std::min<std::uint16_t>(source.channels, destination.channels));
    const std::size_t frames = std::min(std::min<std::size_t>(source.frames, destination.frames),
                                        static_cast<std::size_t>(context.frameCount));
    for (std::uint16_t channel = 0; channel < channels && channel < kMaxChannels; ++channel)
    {
        processChannel(source.channel(channel), destination.channel(channel), frames,
                       states_[channel]);
    }
}

void BiquadProcessor::process(audio::AudioBufferView& buffer) noexcept
{
    if (!buffer.valid())
    {
        return;
    }
    for (std::uint16_t channel = 0; channel < buffer.channels && channel < kMaxChannels; ++channel)
    {
        float* data = buffer.channel(channel);
        processChannel(data, data, buffer.frames, states_[channel]);
    }
}

std::uint32_t BiquadProcessor::latencySamples() const noexcept
{
    return 0;
}

const BiquadCoefficients& BiquadProcessor::coefficients() const noexcept
{
    return coefficients_;
}

} // namespace axf::dsp
