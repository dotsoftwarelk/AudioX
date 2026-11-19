#include "axf/dsp/utility.hpp"

#include <cmath>

namespace axf::dsp
{

void fillSilence(audio::AudioBufferView& buffer) noexcept
{
    if (!buffer.valid())
    {
        return;
    }
    for (core::ChannelCount channel = 0; channel < buffer.channels; ++channel)
    {
        float* data = buffer.channel(channel);
        if (data != nullptr)
        {
            for (std::size_t frame = 0; frame < buffer.frames; ++frame)
            {
                data[frame] = 0.0F;
            }
        }
    }
}

void applyGain(audio::AudioBufferView& buffer, float linearGain) noexcept
{
    if (!buffer.valid())
    {
        return;
    }
    // Non-finite gains are treated as silence rather than propagating NaNs.
    const float safeGain = std::isfinite(linearGain) ? linearGain : 0.0F;
    for (core::ChannelCount channel = 0; channel < buffer.channels; ++channel)
    {
        float* data = buffer.channel(channel);
        if (data == nullptr)
        {
            continue;
        }
        for (std::size_t frame = 0; frame < buffer.frames; ++frame)
        {
            const float sample = data[frame] * safeGain;
            // Flush denormals to zero; avoids large slowdowns on x87/SSE paths.
            data[frame] = std::abs(sample) < 1.0e-30F ? 0.0F : sample;
        }
    }
}

} // namespace axf::dsp
