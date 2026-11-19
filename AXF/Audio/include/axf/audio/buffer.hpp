#pragma once

#include "axf/core/export.hpp"
#include "axf/core/result.hpp"
#include "axf/core/types.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace axf::audio
{

using core::ChannelCount;

/// Planar, preallocated multichannel audio buffer. Construction allocates on
/// the control thread only; all processing methods are allocation-free.
class AudioBuffer final
{
  public:
    AudioBuffer(ChannelCount channels, std::size_t frames);
    AudioBuffer(const AudioBuffer&) = delete;
    AudioBuffer& operator=(const AudioBuffer&) = delete;

    [[nodiscard]] ChannelCount channels() const noexcept
    {
        return channels_;
    }
    [[nodiscard]] std::size_t frames() const noexcept
    {
        return frames_;
    }
    [[nodiscard]] std::size_t sampleCount() const noexcept
    {
        return static_cast<std::size_t>(channels_) * frames_;
    }

    [[nodiscard]] float* channelData(std::uint16_t channel) noexcept;
    [[nodiscard]] const float* channelData(std::uint16_t channel) const noexcept;
    [[nodiscard]] float* const* channelPointers() noexcept
    {
        return channelPointers_.data();
    }
    [[nodiscard]] const float* const* channelPointers() const noexcept
    {
        return channelPointers_.data();
    }

    void clear() noexcept;
    void fill(float value) noexcept;
    [[nodiscard]] core::Result<void> copyFrom(const AudioBuffer& other) noexcept;

  private:
    std::vector<float> samples_;
    std::vector<float*> channelPointers_;
    ChannelCount channels_;
    std::size_t frames_;
};

/// Non-owning view suitable for passing through the audio graph. Does not
/// validate channel counts for you; the graph owns that contract.
struct AudioBufferView final
{
    float* const* channelData{nullptr};
    ChannelCount channels{0};
    std::size_t frames{0};

    [[nodiscard]] float* channel(std::uint16_t index) noexcept
    {
        return channelData == nullptr ? nullptr : channelData[index];
    }
    [[nodiscard]] const float* channel(std::uint16_t index) const noexcept
    {
        return channelData == nullptr ? nullptr : channelData[index];
    }
    [[nodiscard]] bool valid() const noexcept
    {
        return channelData != nullptr && channels > 0 && frames > 0;
    }
};

[[nodiscard]] AXF_API AudioBufferView makeView(AudioBuffer& buffer) noexcept;
[[nodiscard]] AXF_API AudioBufferView makeConstView(const AudioBuffer& buffer) noexcept;

} // namespace axf::audio
