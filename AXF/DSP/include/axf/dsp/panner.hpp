#pragma once

#include "axf/audio/node.hpp"
#include "axf/core/export.hpp"

#include <atomic>

namespace axf::dsp
{

/// Constant-power mono→stereo panner. Pan is atomic so the control thread can
/// move it without allocating or locking in the audio callback.
class PannerProcessor final : public axf::audio::INodeProcessor
{
  public:
    explicit PannerProcessor(float pan = 0.0F) noexcept;

    void prepare(double sampleRate, std::uint32_t maxFrames) noexcept override;
    void reset() noexcept override;
    void process(axf::audio::ProcessContext& context) noexcept override;
    [[nodiscard]] std::uint32_t latencySamples() const noexcept override;

    /// -1 = hard left, 0 = centre, +1 = hard right.
    void setPan(float pan) noexcept;
    [[nodiscard]] float pan() const noexcept;

  private:
    std::atomic<float> pan_;
};

} // namespace axf::dsp
