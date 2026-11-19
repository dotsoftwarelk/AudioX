#pragma once

#include "axf/core/export.hpp"
#include "axf/core/result.hpp"
#include "axf/core/types.hpp"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace axf::media
{

/// Preallocated interleaved recorder. `capture()` is real-time safe: it only
/// copies into memory reserved by `start()` and never allocates or locks.
/// File I/O happens in `stopAndWrite()` on the control thread.
class AudioRecorder final
{
  public:
    AudioRecorder() = default;
    ~AudioRecorder() = default;
    AudioRecorder(const AudioRecorder&) = delete;
    AudioRecorder& operator=(const AudioRecorder&) = delete;

    /// Allocates the capture buffer. Control-thread only.
    [[nodiscard]] core::Result<void> start(core::SampleRate sampleRate, core::ChannelCount channels,
                                           std::uint32_t maxFrames) noexcept;
    void stop() noexcept;
    void reset() noexcept;

    void capture(float* const* inputs, std::uint32_t frames) noexcept;

    [[nodiscard]] core::Result<void> stopAndWrite(const std::string& path) noexcept;

    [[nodiscard]] bool isRecording() const noexcept;
    [[nodiscard]] std::uint64_t capturedFrames() const noexcept;
    [[nodiscard]] std::uint64_t droppedFrames() const noexcept;
    [[nodiscard]] std::uint32_t capacityFrames() const noexcept;
    [[nodiscard]] core::ChannelCount channels() const noexcept;
    [[nodiscard]] core::SampleRate sampleRate() const noexcept;

  private:
    std::vector<float> samples_;
    std::atomic<bool> recording_{false};
    std::atomic<std::uint64_t> framesCaptured_{0};
    std::atomic<std::uint64_t> framesDropped_{0};
    std::uint32_t capacity_{0};
    core::ChannelCount channels_{0};
    core::SampleRate sampleRate_{0};
};

} // namespace axf::media
