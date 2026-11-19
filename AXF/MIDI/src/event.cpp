#include "axf/midi/event.hpp"

namespace axf::midi
{
namespace
{

constexpr std::uint8_t kChannelMask = 0x0F;
constexpr std::uint8_t kStatusMask = 0xF0;

MidiEvent channelEvent(std::uint64_t frame, std::uint8_t status, std::uint8_t channel,
                       std::uint8_t data1, std::uint8_t data2) noexcept
{
    return MidiEvent{frame, static_cast<std::uint8_t>(status | (channel & kChannelMask)),
                     static_cast<std::uint8_t>(data1 & 0x7F),
                     static_cast<std::uint8_t>(data2 & 0x7F)};
}

} // namespace

MidiEvent makeNoteOn(std::uint64_t frame, std::uint8_t channel, std::uint8_t note,
                     std::uint8_t velocity) noexcept
{
    return channelEvent(frame, 0x90, channel, note, velocity);
}

MidiEvent makeNoteOff(std::uint64_t frame, std::uint8_t channel, std::uint8_t note,
                      std::uint8_t velocity) noexcept
{
    return channelEvent(frame, 0x80, channel, note, velocity);
}

MidiEvent makeControlChange(std::uint64_t frame, std::uint8_t channel, std::uint8_t controller,
                            std::uint8_t value) noexcept
{
    return channelEvent(frame, 0xB0, channel, controller, value);
}

MidiMessageType messageType(const MidiEvent& event) noexcept
{
    switch (event.status & kStatusMask)
    {
    case 0x80:
        return MidiMessageType::NoteOff;
    case 0x90:
        return MidiMessageType::NoteOn;
    case 0xB0:
        return MidiMessageType::ControlChange;
    case 0xC0:
        return MidiMessageType::ProgramChange;
    case 0xE0:
        return MidiMessageType::PitchBend;
    case 0xA0:
        return MidiMessageType::Aftertouch;
    default:
        return event.status < 0x80 ? MidiMessageType::System : MidiMessageType::Unknown;
    }
}

std::uint8_t eventChannel(const MidiEvent& event) noexcept
{
    return static_cast<std::uint8_t>(event.status & kChannelMask);
}

} // namespace axf::midi
