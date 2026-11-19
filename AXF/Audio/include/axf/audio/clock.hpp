#pragma once

#include "axf/core/export.hpp"
#include "axf/core/types.hpp"

#include <atomic>

namespace axf::audio
{

/// Monotonic frame counter with time conversions. Reads are safe from any
/// thread; writes belong to the audio thread (or control thread when stopped).
class AudioClock final
{
  public:
    explicit AudioClock(core::SampleRate sampleRate) noexcept
        : sampleRate_(sampleRate > 0 ? sampleRate : 48000), frame_(0)
    {
    }

    [[nodiscard]] core::SampleRate sampleRate() const noexcept
    {
        return sampleRate_.load(std::memory_order_relaxed);
    }

    void setSampleRate(core::SampleRate rate) noexcept
    {
        sampleRate_.store(rate > 0 ? rate : 48000, std::memory_order_relaxed);
    }

    [[nodiscard]] core::FrameIndex currentFrame() const noexcept
    {
        return frame_.load(std::memory_order_relaxed);
    }

    [[nodiscard]] double currentSeconds() const noexcept
    {
        return core::framesToSeconds(currentFrame(), sampleRate());
    }

    void advance(core::FrameIndex frames) noexcept
    {
        if (frames > 0)
        {
            frame_.fetch_add(frames, std::memory_order_relaxed);
        }
    }

    void setFrame(core::FrameIndex frame) noexcept
    {
        frame_.store(frame < 0 ? 0 : frame, std::memory_order_relaxed);
    }

  private:
    std::atomic<core::SampleRate> sampleRate_;
    std::atomic<core::FrameIndex> frame_;
};

} // namespace axf::audio
