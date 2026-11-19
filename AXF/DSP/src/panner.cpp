#include "axf/dsp/panner.hpp"

#include <algorithm>
#include <cmath>

namespace axf::dsp
{
namespace
{
constexpr double kQuarterPi = 3.14159265358979323846 / 4.0;

float clampPan(float value) noexcept
{
    if (!std::isfinite(value))
    {
        return 0.0F;
    }
    return std::clamp(value, -1.0F, 1.0F);
}
} // namespace

PannerProcessor::PannerProcessor(float pan) noexcept : pan_(clampPan(pan))
{
}

void PannerProcessor::prepare(double, std::uint32_t) noexcept
{
}

void PannerProcessor::reset() noexcept
{
    pan_.store(0.0F, std::memory_order_relaxed);
}

void PannerProcessor::process(audio::ProcessContext& context) noexcept
{
    if (context.inputCount == 0 || context.outputCount < 2 || context.inputs == nullptr ||
        context.outputs == nullptr)
    {
        return;
    }
    const audio::AudioBufferView& source = context.inputs[0];
    if (source.channels == 0 || source.frames == 0)
    {
        return;
    }
    const float* input = source.channel(0);
    float* left = context.outputs[0].channel(0);
    float* right = context.outputs[1].channel(0);
    if (input == nullptr || left == nullptr || right == nullptr)
    {
        return;
    }

    const float pan = clampPan(pan_.load(std::memory_order_relaxed));
    const double angle = (static_cast<double>(pan) + 1.0) * kQuarterPi;
    const float leftGain = static_cast<float>(std::cos(angle));
    const float rightGain = static_cast<float>(std::sin(angle));
    const std::size_t frames = std::min<std::size_t>(
        source.frames, std::min<std::size_t>(context.outputs[0].frames, context.outputs[1].frames));
    for (std::size_t frame = 0; frame < frames; ++frame)
    {
        left[frame] = input[frame] * leftGain;
        right[frame] = input[frame] * rightGain;
    }
}

std::uint32_t PannerProcessor::latencySamples() const noexcept
{
    return 0;
}

void PannerProcessor::setPan(float pan) noexcept
{
    pan_.store(clampPan(pan), std::memory_order_relaxed);
}

float PannerProcessor::pan() const noexcept
{
    return pan_.load(std::memory_order_relaxed);
}

} // namespace axf::dsp
