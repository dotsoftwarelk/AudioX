#include "axf/media/recorder.hpp"

#include "axf/media/wav_file.hpp"

#include <algorithm>
#include <limits>

namespace axf::media
{

core::Result<void> AudioRecorder::start(core::SampleRate sampleRate, core::ChannelCount channels,
                                        std::uint32_t maxFrames) noexcept
{
    if (!core::isValidSampleRate(sampleRate) || channels == 0 || channels > 512 || maxFrames == 0)
    {
        return core::makeError(core::ErrorCode::OutOfRange, "invalid recording configuration");
    }
    if (recording_.load(std::memory_order_acquire))
    {
        return core::makeError(core::ErrorCode::AlreadyExists, "recording is already active");
    }
    const std::size_t count = static_cast<std::size_t>(maxFrames) * channels;
    if (count > samples_.max_size())
    {
        return core::makeError(core::ErrorCode::OutOfMemory, "recording buffer too large");
    }
    try
    {
        samples_.assign(count, 0.0F);
    }
    catch (...)
    {
        return core::makeError(core::ErrorCode::OutOfMemory, "recording allocation failed");
    }
    capacity_ = maxFrames;
    channels_ = channels;
    sampleRate_ = sampleRate;
    framesCaptured_.store(0, std::memory_order_release);
    framesDropped_.store(0, std::memory_order_release);
    recording_.store(true, std::memory_order_release);
    return core::success();
}

void AudioRecorder::stop() noexcept
{
    recording_.store(false, std::memory_order_release);
}

void AudioRecorder::reset() noexcept
{
    stop();
    framesCaptured_.store(0, std::memory_order_release);
    framesDropped_.store(0, std::memory_order_release);
    capacity_ = 0;
    channels_ = 0;
    sampleRate_ = 0;
    samples_.clear();
}

void AudioRecorder::capture(float* const* inputs, std::uint32_t frames) noexcept
{
    if (!recording_.load(std::memory_order_acquire) || inputs == nullptr || capacity_ == 0)
    {
        return;
    }
    const std::uint64_t used = framesCaptured_.load(std::memory_order_relaxed);
    if (used >= capacity_)
    {
        framesDropped_.fetch_add(frames, std::memory_order_relaxed);
        return;
    }
    const std::uint32_t available = capacity_ - static_cast<std::uint32_t>(used);
    const std::uint32_t count = std::min(frames, available);
    if (count < frames)
    {
        framesDropped_.fetch_add(frames - count, std::memory_order_relaxed);
    }
    for (std::uint32_t frame = 0; frame < count; ++frame)
    {
        const std::size_t destination = (static_cast<std::size_t>(used) + frame) * channels_;
        for (core::ChannelCount channel = 0; channel < channels_; ++channel)
        {
            const float* source = inputs[channel];
            samples_[destination + channel] = source == nullptr ? 0.0F : source[frame];
        }
    }
    framesCaptured_.store(used + count, std::memory_order_release);
}

core::Result<void> AudioRecorder::stopAndWrite(const std::string& path) noexcept
{
    if (!recording_.exchange(false, std::memory_order_acq_rel))
    {
        return core::makeError(core::ErrorCode::InvalidArgument, "recording is not active");
    }
    AudioFileInfo info{AudioFileFormat::Wav, sampleRate_, channels_,
                       framesCaptured_.load(std::memory_order_acquire)};
    const auto written =
        writeWavFile(path, info, samples_.data(), static_cast<std::size_t>(info.frames));
    if (!written)
    {
        return written.error();
    }
    return core::success();
}

bool AudioRecorder::isRecording() const noexcept
{
    return recording_.load(std::memory_order_acquire);
}

std::uint64_t AudioRecorder::capturedFrames() const noexcept
{
    return framesCaptured_.load(std::memory_order_acquire);
}

std::uint64_t AudioRecorder::droppedFrames() const noexcept
{
    return framesDropped_.load(std::memory_order_acquire);
}

std::uint32_t AudioRecorder::capacityFrames() const noexcept
{
    return capacity_;
}

core::ChannelCount AudioRecorder::channels() const noexcept
{
    return channels_;
}

core::SampleRate AudioRecorder::sampleRate() const noexcept
{
    return sampleRate_;
}

} // namespace axf::media
