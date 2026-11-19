#pragma once

#include "axf/core/export.hpp"
#include "axf/core/types.hpp"

#include <cstdint>
#include <string>

namespace axf::core::time
{

constexpr double kDefaultPpq = 960.0;

[[nodiscard]] constexpr bool isValidTempo(double bpm) noexcept
{
    return std::isfinite(bpm) && bpm > 0.0;
}

[[nodiscard]] inline double beatsToSeconds(double beats, double bpm) noexcept
{
    return isValidTempo(bpm) ? beats * 60.0 / bpm : 0.0;
}

[[nodiscard]] inline double secondsToBeats(double seconds, double bpm) noexcept
{
    return isValidTempo(bpm) ? seconds * bpm / 60.0 : 0.0;
}

[[nodiscard]] inline FrameIndex beatsToFrames(double beats, double bpm, SampleRate rate) noexcept
{
    return secondsToFrames(beatsToSeconds(beats, bpm), rate);
}

[[nodiscard]] inline double framesToBeats(FrameIndex frames, double bpm, SampleRate rate) noexcept
{
    return secondsToBeats(framesToSeconds(frames, rate), bpm);
}

[[nodiscard]] inline std::uint64_t beatsToTicks(double beats, double ppq) noexcept
{
    if (!std::isfinite(beats) || !std::isfinite(ppq) || ppq <= 0.0)
    {
        return 0;
    }
    const double ticks = beats * ppq;
    return ticks <= 0.0 ? 0U : static_cast<std::uint64_t>(ticks + 0.5);
}

[[nodiscard]] inline double ticksToBeats(std::uint64_t ticks, double ppq) noexcept
{
    return ppq > 0.0 ? static_cast<double>(ticks) / ppq : 0.0;
}

/// Allocating helper for UI display. Never call from the audio callback.
[[nodiscard]] AXF_API std::string formatTimecode(FrameIndex frame, SampleRate rate);

} // namespace axf::core::time
