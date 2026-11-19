#include "axf/devices/device.hpp"

#include "axf/core/types.hpp"

namespace axf::devices
{

const char* deviceStateName(DeviceState state) noexcept
{
    switch (state)
    {
    case DeviceState::Unknown:
        return "unknown";
    case DeviceState::Active:
        return "active";
    case DeviceState::Disabled:
        return "disabled";
    case DeviceState::Disconnected:
        return "disconnected";
    case DeviceState::NotPresent:
        return "not present";
    }
    return "unknown";
}

const char* backendName(Backend backend) noexcept
{
    switch (backend)
    {
    case Backend::None:
        return "none";
    case Backend::WasapiShared:
        return "WASAPI shared";
    case Backend::WasapiExclusive:
        return "WASAPI exclusive";
    case Backend::Asio:
        return "ASIO";
    }
    return "unknown";
}

core::Result<void> validateConfig(const DeviceConfig& config) noexcept
{
    if (!core::isValidSampleRate(config.sampleRate) || config.sampleRate < 8000 ||
        config.sampleRate > 384000)
    {
        return core::makeError(core::ErrorCode::OutOfRange,
                               "sample rate must be between 8000 and 384000 Hz");
    }
    if (!core::isPowerOfTwo(config.bufferFrames) || config.bufferFrames < 16 ||
        config.bufferFrames > 8192)
    {
        return core::makeError(core::ErrorCode::OutOfRange,
                               "buffer frames must be a power of two between 16 and 8192");
    }
    if (config.inputChannels > 512 || config.outputChannels > 512)
    {
        return core::makeError(core::ErrorCode::OutOfRange, "channel count exceeds 512");
    }
    if (config.inputChannels == 0 && config.outputChannels == 0)
    {
        return core::makeError(core::ErrorCode::InvalidArgument,
                               "at least one input or output channel is required");
    }
    if (config.backend == Backend::None)
    {
        return core::makeError(core::ErrorCode::InvalidArgument, "no audio backend selected");
    }
    return core::success();
}

core::Result<void> validateAsioConfig(const DeviceConfig& config) noexcept
{
    const auto validation = validateConfig(config);
    if (!validation)
    {
        return validation.error();
    }
    if (config.backend != Backend::Asio)
    {
        return core::makeError(core::ErrorCode::InvalidArgument,
                               "ASIO requires an ASIO backend selection");
    }
    if (config.inputChannels > 512 || config.outputChannels > 512)
    {
        return core::makeError(core::ErrorCode::OutOfRange, "ASIO channel count exceeds 512");
    }
    return core::success();
}

core::Result<void> validateWasapiConfig(const DeviceConfig& config) noexcept
{
    const auto validation = validateConfig(config);
    if (!validation)
    {
        return validation.error();
    }
    if (config.backend != Backend::WasapiShared && config.backend != Backend::WasapiExclusive)
    {
        return core::makeError(core::ErrorCode::InvalidArgument,
                               "WASAPI requires a WASAPI backend selection");
    }
    if (config.inputChannels > 0 && config.outputChannels > 0)
    {
        return core::makeError(core::ErrorCode::Unsupported,
                               "full duplex requires separate capture and render devices");
    }
    if (config.backend == Backend::WasapiExclusive && config.outputChannels > 2)
    {
        return core::makeError(core::ErrorCode::Unsupported,
                               "exclusive mode supports at most two channels in Phase 3");
    }
    return core::success();
}

} // namespace axf::devices
