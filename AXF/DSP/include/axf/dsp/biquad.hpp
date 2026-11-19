#pragma once

#include "axf/audio/node.hpp"
#include "axf/core/export.hpp"
#include "axf/core/result.hpp"
#include "axf/dsp/processor.hpp"

#include <array>
#include <cstdint>

namespace axf::dsp
{

enum class BiquadType : std::uint8_t
{
    LowPass,
    HighPass,
    Peaking,
};

struct BiquadCoefficients final
{
    double b0{1.0};
    double b1{0.0};
    double b2{0.0};
    double a1{0.0};
    double a2{0.0};
};

struct BiquadDesign final
{
    BiquadType type{BiquadType::LowPass};
    double frequencyHz{1000.0};
    double q{0.7071067811865476};
    double gainDb{0.0};
};

/// RBJ cookbook coefficients, designed on the control thread. Only `frequencyHz`
/// is used for LowPass/HighPass; Peaking also uses `gainDb`.
[[nodiscard]] AXF_API core::Result<BiquadCoefficients> designBiquad(const BiquadDesign& design,
                                                                    double sampleRate) noexcept;

/// Allocation-free biquad with immutable coefficients. Up to 8 channels are
/// supported; additional channels are passed through unchanged. Changing the
/// filter means constructing a new processor and republishing the graph.
class BiquadProcessor final : public audio::INodeProcessor, public IProcessor
{
  public:
    static constexpr std::uint16_t kMaxChannels = 8;

    explicit BiquadProcessor(BiquadCoefficients coefficients) noexcept;

    void prepare(double sampleRate, std::uint32_t maxFrames) noexcept override;
    void reset() noexcept override;
    void process(audio::ProcessContext& context) noexcept override;
    void process(audio::AudioBufferView& buffer) noexcept override;
    [[nodiscard]] std::uint32_t latencySamples() const noexcept override;

    [[nodiscard]] const BiquadCoefficients& coefficients() const noexcept;

  private:
    struct State final
    {
        double x1{0.0};
        double x2{0.0};
        double y1{0.0};
        double y2{0.0};
    };

    void processChannel(const float* source, float* destination, std::size_t frames,
                        State& state) const noexcept;

    BiquadCoefficients coefficients_;
    std::array<State, kMaxChannels> states_{};
};

} // namespace axf::dsp
