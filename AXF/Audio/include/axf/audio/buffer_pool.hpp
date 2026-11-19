#pragma once

#include "axf/audio/buffer.hpp"
#include "axf/core/export.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace axf::audio
{

/// Preallocates all audio buffers at compile time. No buffer is created or
/// destroyed on the audio thread.
class AudioBufferPool final
{
  public:
    AudioBufferPool(core::ChannelCount channels, std::uint32_t frames, std::size_t count);

    [[nodiscard]] AudioBuffer& get(std::size_t index) noexcept
    {
        return *buffers_[index];
    }

    [[nodiscard]] const AudioBuffer& get(std::size_t index) const noexcept
    {
        return *buffers_[index];
    }

    [[nodiscard]] AudioBuffer* tryGet(std::size_t index) noexcept
    {
        return index < buffers_.size() ? buffers_[index].get() : nullptr;
    }

    [[nodiscard]] std::size_t size() const noexcept
    {
        return buffers_.size();
    }

  private:
    std::vector<std::unique_ptr<AudioBuffer>> buffers_;
};

} // namespace axf::audio
