#pragma once

#include "axf/core/result.hpp"
#include "axf/midi/event.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace axf::midi
{

/// Standard MIDI File format 0 (single track). Events carry absolute sample
/// frames; conversion uses the file's tempo and PPQ division.
struct SmfDocument final
{
    std::uint16_t division{480};             ///< Ticks per quarter note.
    std::uint32_t tempoUsPerQuarter{500000}; ///< 120 BPM default.
    std::vector<MidiEvent> events;           ///< Sorted by frame.
};

/// Parses format 0/1_SMf bytes. Only the first track is imported.
[[nodiscard]] AXF_API core::Result<SmfDocument> readSmf(const std::vector<std::uint8_t>& bytes,
                                                        double sampleRate) noexcept;

/// Serialises a format 0 file with one track and one tempo event.
[[nodiscard]] AXF_API core::Result<std::vector<std::uint8_t>> writeSmf(const SmfDocument& document,
                                                                       double sampleRate) noexcept;

} // namespace axf::midi
