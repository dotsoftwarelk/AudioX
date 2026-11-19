#include "axf/audio/buffer.hpp"
#include "axf/audio/transport.hpp"

#include <algorithm>
#include <cstring>

namespace axf::audio
{

AudioBuffer::AudioBuffer(ChannelCount channels, std::size_t frames)
    : samples_(static_cast<std::size_t>(channels) * frames, 0.0F), channels_(channels),
      frames_(frames)
{
    channelPointers_.reserve(channels);
    for (std::size_t channel = 0; channel < static_cast<std::size_t>(channels); ++channel)
    {
        channelPointers_.push_back(samples_.data() + channel * frames);
    }
}

float* AudioBuffer::channelData(std::uint16_t channel) noexcept
{
    if (channel >= channels_)
    {
        return nullptr;
    }
    return channelPointers_[channel];
}

const float* AudioBuffer::channelData(std::uint16_t channel) const noexcept
{
    if (channel >= channels_)
    {
        return nullptr;
    }
    return channelPointers_[channel];
}

void AudioBuffer::clear() noexcept
{
    std::fill(samples_.begin(), samples_.end(), 0.0F);
}

void AudioBuffer::fill(float value) noexcept
{
    std::fill(samples_.begin(), samples_.end(), value);
}

core::Result<void> AudioBuffer::copyFrom(const AudioBuffer& other) noexcept
{
    if (channels_ != other.channels_ || frames_ != other.frames_)
    {
        return core::makeError(core::ErrorCode::InvalidArgument,
                               "audio buffer dimensions do not match");
    }
    if (!samples_.empty())
    {
        std::memcpy(samples_.data(), other.samples_.data(), samples_.size() * sizeof(float));
    }
    return core::success();
}

AudioBufferView makeView(AudioBuffer& buffer) noexcept
{
    return AudioBufferView{buffer.channelPointers(), buffer.channels(), buffer.frames()};
}

AudioBufferView makeConstView(const AudioBuffer& buffer) noexcept
{
    return AudioBufferView{const_cast<float* const*>(buffer.channelPointers()), buffer.channels(),
                           buffer.frames()};
}

void advanceTransport(Transport& transport, core::FrameIndex frames) noexcept
{
    if (frames <= 0 || !transport.isRolling())
    {
        return;
    }
    transport.setPosition(transport.position() + frames);
}

const char* transportStateName(Transport::State state) noexcept
{
    switch (state)
    {
    case Transport::State::Stopped:
        return "stopped";
    case Transport::State::Playing:
        return "playing";
    case Transport::State::Paused:
        return "paused";
    case Transport::State::Recording:
        return "recording";
    }
    return "unknown";
}

} // namespace axf::audio
