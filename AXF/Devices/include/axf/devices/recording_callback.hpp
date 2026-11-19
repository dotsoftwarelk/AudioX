#pragma once

#include "axf/devices/device.hpp"
#include "axf/media/recorder.hpp"

namespace axf::devices
{

/// Forwards every device callback to the engine and records the input buffers
/// before processing. The recorder must already be started on the control
/// thread; both objects must outlive the device stream.
class RecordingDeviceCallback final : public IAudioDeviceCallback
{
  public:
    RecordingDeviceCallback(IAudioDeviceCallback& next, media::AudioRecorder& recorder) noexcept
        : next_(next), recorder_(recorder)
    {
    }

    void onAudioCallback(float* const* inputs, float* const* outputs, std::uint32_t frameCount,
                         std::uint64_t streamFramePosition) noexcept override
    {
        recorder_.capture(inputs, frameCount);
        next_.onAudioCallback(inputs, outputs, frameCount, streamFramePosition);
    }

  private:
    IAudioDeviceCallback& next_;
    media::AudioRecorder& recorder_;
};

} // namespace axf::devices
