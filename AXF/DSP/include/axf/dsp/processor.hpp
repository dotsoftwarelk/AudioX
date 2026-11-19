#pragma once

#include "axf/audio/buffer.hpp"

#include <cstdint>

namespace axf::dsp
{

/// Common processor contract. All methods are called on the audio thread and
/// must satisfy the AXF real-time rules. Implementations own their state and
/// must not allocate after prepare().
class IProcessor
{
  public:
    virtual ~IProcessor() = default;
    virtual void prepare(double sampleRate, std::uint32_t maxFrames) noexcept = 0;
    virtual void reset() noexcept = 0;
    virtual void process(audio::AudioBufferView& buffer) noexcept = 0;
};

} // namespace axf::dsp
