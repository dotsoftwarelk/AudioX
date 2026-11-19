#pragma once

#include "axf/core/export.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace axf::core
{

using SampleRate = std::int32_t;    ///< Frames per second; must be positive.
using ChannelCount = std::uint16_t; ///< Number of channels in a buffer or device.
using FrameIndex = std::int64_t;    ///< Monotonic sample-frame position.
using SampleIndex = std::int64_t;

constexpr FrameIndex kInvalidFrame = -1;

enum class ChannelLayout : std::uint8_t
{
    Mono,
    Stereo,
    Multichannel,
};

struct Version final
{
    std::uint16_t major{0};
    std::uint16_t minor{0};
    std::uint16_t patch{0};

    friend bool operator==(const Version&, const Version&) noexcept = default;
};

constexpr Version kAxfVersion{0, 1, 0};

[[nodiscard]] constexpr bool isPowerOfTwo(std::size_t value) noexcept
{
    return value != 0 && (value & (value - 1)) == 0;
}

[[nodiscard]] constexpr bool isValidSampleRate(SampleRate rate) noexcept
{
    return rate > 0;
}

[[nodiscard]] inline double framesToSeconds(FrameIndex frames, SampleRate rate) noexcept
{
    if (!isValidSampleRate(rate))
    {
        return 0.0;
    }
    return static_cast<double>(frames) / static_cast<double>(rate);
}

[[nodiscard]] inline FrameIndex secondsToFrames(double seconds, SampleRate rate) noexcept
{
    if (!isValidSampleRate(rate) || !std::isfinite(seconds))
    {
        return 0;
    }
    const double value = seconds * static_cast<double>(rate);
    if (value >= static_cast<double>(std::numeric_limits<FrameIndex>::max()))
    {
        return std::numeric_limits<FrameIndex>::max();
    }
    if (value <= static_cast<double>(std::numeric_limits<FrameIndex>::min()))
    {
        return std::numeric_limits<FrameIndex>::min();
    }
    return static_cast<FrameIndex>(std::llround(value));
}

[[nodiscard]] inline float dbToLinear(float decibels) noexcept
{
    if (!std::isfinite(decibels))
    {
        return 0.0F;
    }
    return std::pow(10.0F, decibels / 20.0F);
}

[[nodiscard]] inline float linearToDb(float linear) noexcept
{
    if (!std::isfinite(linear) || linear <= 0.0F)
    {
        return -std::numeric_limits<float>::infinity();
    }
    return 20.0F * std::log10(linear);
}

template <class T> [[nodiscard]] constexpr T clampValue(T value, T lower, T upper) noexcept
{
    return std::clamp(value, lower, upper);
}

} // namespace axf::core
