#include "axf/dsp/gain_processor.hpp"
#include "axf/dsp/utility.hpp"

#include <cmath>

namespace axf::dsp
{

GainProcessor::GainProcessor(float linearGain) noexcept : gain_(linearGain)
{
}

void GainProcessor::prepare(double, std::uint32_t) noexcept
{
}

void GainProcessor::reset() noexcept
{
    gain_.store(1.0F, std::memory_order_relaxed);
}

void GainProcessor::process(axf::audio::ProcessContext& context) noexcept
{
    if (context.inputCount == 0 || context.outputCount == 0 || context.inputs == nullptr ||
        context.outputs == nullptr)
    {
        return;
    }

    const axf::audio::AudioBufferView& source = context.inputs[0];
    axf::audio::AudioBufferView& destination = context.outputs[0];
    const axf::core::ChannelCount channels =
        source.channels < destination.channels ? source.channels : destination.channels;
    const std::size_t frames =
        source.frames < destination.frames ? source.frames : destination.frames;
    const float linearGain = gain_.load(std::memory_order_relaxed);

    for (axf::core::ChannelCount channel = 0; channel < channels; ++channel)
    {
        const float* from = source.channel(channel);
        float* to = destination.channel(channel);
        if (from == nullptr || to == nullptr)
        {
            continue;
        }
        for (std::size_t frame = 0; frame < frames; ++frame)
        {
            const float value = from[frame] * linearGain;
            to[frame] = std::abs(value) < 1.0e-30F ? 0.0F : value;
        }
    }
}

std::uint32_t GainProcessor::latencySamples() const noexcept
{
    return 0;
}

void GainProcessor::setGain(float linearGain) noexcept
{
    gain_.store(linearGain, std::memory_order_relaxed);
}

float GainProcessor::gain() const noexcept
{
    return gain_.load(std::memory_order_relaxed);
}

} // namespace axf::dsp
