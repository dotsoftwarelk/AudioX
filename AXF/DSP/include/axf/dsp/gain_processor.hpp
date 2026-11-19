#pragma once

#include "axf/audio/node.hpp"
#include "axf/core/export.hpp"

#include <atomic>

namespace axf::dsp
{

/// Real-time-safe gain node. The gain is set from the control thread and read
/// atomically in process(); no allocation or locking occurs in the callback.
class GainProcessor final : public axf::audio::INodeProcessor
{
  public:
    explicit GainProcessor(float linearGain = 1.0F) noexcept;

    void prepare(double sampleRate, std::uint32_t maxFrames) noexcept override;
    void reset() noexcept override;
    void process(axf::audio::ProcessContext& context) noexcept override;
    [[nodiscard]] std::uint32_t latencySamples() const noexcept override;

    void setGain(float linearGain) noexcept;
    [[nodiscard]] float gain() const noexcept;

  private:
    std::atomic<float> gain_;
};

} // namespace axf::dsp
