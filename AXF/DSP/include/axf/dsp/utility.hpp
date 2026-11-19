#pragma once

#include "axf/audio/buffer.hpp"
#include "axf/core/export.hpp"

namespace axf::dsp
{

/// Minimal, allocation-free primitives. The Phase 4 catalogue adds biquad
/// filters/EQ and constant-power panning; compressor, limiter and effects
/// remain future work.
AXF_API void fillSilence(audio::AudioBufferView& buffer) noexcept;
AXF_API void applyGain(audio::AudioBufferView& buffer, float linearGain) noexcept;

} // namespace axf::dsp
