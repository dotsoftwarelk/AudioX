#pragma once

#include "axf/core/export.hpp"
#include "axf/core/result.hpp"
#include "axf/core/types.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace axf::devices
{

enum class Backend : std::uint8_t
{
    None,
    WasapiShared,
    WasapiExclusive,
    Asio
};

[[nodiscard]] AXF_API const char* backendName(Backend backend) noexcept;

struct DeviceInfo final
{
    std::string id;
    std::string name;
    Backend backend{Backend::None};
    core::ChannelCount maxInputChannels{0};
    core::ChannelCount maxOutputChannels{0};
    std::vector<core::SampleRate> supportedSampleRates;
};

struct DeviceConfig final
{
    core::SampleRate sampleRate{48000};
    core::ChannelCount inputChannels{0};
    core::ChannelCount outputChannels{2};
    std::uint32_t bufferFrames{256};
    Backend backend{Backend::WasapiShared};
};

/// Validates without touching hardware. Control-thread only.
[[nodiscard]] AXF_API core::Result<void> validateConfig(const DeviceConfig& config) noexcept;

/// Called on the audio device thread. All real-time rules apply to the
/// implementation of this callback: no allocation, locks, I/O or UI.
class IAudioDeviceCallback
{
  public:
    virtual ~IAudioDeviceCallback() = default;
    virtual void onAudioCallback(float* const* inputs, float* const* outputs,
                                 std::uint32_t frameCount,
                                 std::uint64_t streamFramePosition) noexcept = 0;
};

class IAudioDevice
{
  public:
    virtual ~IAudioDevice() = default;
    [[nodiscard]] virtual const DeviceInfo& info() const noexcept = 0;
    [[nodiscard]] virtual const DeviceConfig& config() const noexcept = 0;
    [[nodiscard]] virtual core::Result<void> start(IAudioDeviceCallback* callback) = 0;
    virtual void stop() noexcept = 0;
    [[nodiscard]] virtual bool isRunning() const noexcept = 0;
    [[nodiscard]] virtual double reportedLatencySeconds() const noexcept = 0;
};

/// Backend implementations (WASAPI in Phase 3, ASIO afterwards) satisfy this
/// interface. No backend is compiled in Phase 0.
class IDeviceBackend
{
  public:
    virtual ~IDeviceBackend() = default;
    [[nodiscard]] virtual Backend backend() const noexcept = 0;
    [[nodiscard]] virtual core::Result<std::vector<DeviceInfo>> enumerate() = 0;
    [[nodiscard]] virtual core::Result<std::unique_ptr<IAudioDevice>> open(
        const std::string& deviceId, const DeviceConfig& config) = 0;
};

} // namespace axf::devices
