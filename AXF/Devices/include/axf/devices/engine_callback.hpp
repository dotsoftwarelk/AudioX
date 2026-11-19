#pragma once

#include "axf/audio/engine.hpp"
#include "axf/devices/device.hpp"

namespace axf::devices
{

/// Device callback adapter: forwards a device's planar buffers directly into
/// the real-time engine. Keep one instance per engine/device pair; the engine
/// must outlive the adapter and the running device.
class EngineDeviceCallback final : public IAudioDeviceCallback
{
  public:
    explicit EngineDeviceCallback(audio::AudioEngine& engine) noexcept : engine_(engine)
    {
    }

    void onAudioCallback(float* const* inputs, float* const* outputs, std::uint32_t frameCount,
                         std::uint64_t streamFramePosition) noexcept override
    {
        engine_.process(inputs, outputs, frameCount, streamFramePosition);
    }

  private:
    audio::AudioEngine& engine_;
};

} // namespace axf::devices
