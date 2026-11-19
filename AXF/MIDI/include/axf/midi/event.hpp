#pragma once

#include "axf/core/export.hpp"

#include <cstddef>
#include <cstdint>

namespace axf::midi
{

struct MidiEvent final
{
    std::uint64_t frame{0}; ///< Scheduled sample frame.
    std::uint8_t status{0};
    std::uint8_t data1{0};
    std::uint8_t data2{0};
};

/// Non-owning, preallocated event view. Pushing never allocates.
struct MidiEventBufferView final
{
    MidiEvent* data{nullptr};
    std::size_t size{0};
    std::size_t capacity{0};

    [[nodiscard]] bool push(const MidiEvent& event) noexcept
    {
        if (data == nullptr || size >= capacity)
        {
            return false;
        }
        data[size++] = event;
        return true;
    }
    void clear() noexcept
    {
        size = 0;
    }
};

enum class MidiMessageType : std::uint8_t
{
    NoteOff,
    NoteOn,
    ControlChange,
    ProgramChange,
    PitchBend,
    Aftertouch,
    System,
    Unknown,
};

[[nodiscard]] AXF_API MidiEvent makeNoteOn(std::uint64_t frame, std::uint8_t channel,
                                           std::uint8_t note, std::uint8_t velocity) noexcept;
[[nodiscard]] AXF_API MidiEvent makeNoteOff(std::uint64_t frame, std::uint8_t channel,
                                            std::uint8_t note, std::uint8_t velocity) noexcept;
[[nodiscard]] AXF_API MidiEvent makeControlChange(std::uint64_t frame, std::uint8_t channel,
                                                  std::uint8_t controller,
                                                  std::uint8_t value) noexcept;
[[nodiscard]] AXF_API MidiMessageType messageType(const MidiEvent& event) noexcept;
[[nodiscard]] AXF_API std::uint8_t eventChannel(const MidiEvent& event) noexcept;

} // namespace axf::midi
