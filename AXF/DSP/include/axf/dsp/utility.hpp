#pragma once

#include "axf/audio/buffer.hpp"
#include "axf/core/export.hpp"

namespace axf::dsp
{

/// Minimal, allocation-free primitives used by Phase 2 integration tests.
/// The full DSP catalogue (filters, EQ, compressor, limiter, effects) is
/// specified for Phase 4.
AXF_API void fillSilence(audio::AudioBufferView& buffer) noexcept;
AXF_API void applyGain(audio::AudioBufferView& buffer, float linearGain) noexcept;

} // namespace axf::dsp
