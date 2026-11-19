#include "axf/devices/device.hpp"

#include "axf/core/types.hpp"

namespace axf::devices
{

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

} // namespace axf::devices
