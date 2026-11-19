#include "axf/midi/smf.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

namespace axf::midi
{
namespace
{
constexpr double kMicrosecondsPerSecond = 1000000.0;

bool appendU32(std::vector<std::uint8_t>& bytes, std::uint32_t value) noexcept
{
    bytes.push_back(static_cast<std::uint8_t>(value >> 24U));
    bytes.push_back(static_cast<std::uint8_t>(value >> 16U));
    bytes.push_back(static_cast<std::uint8_t>(value >> 8U));
    bytes.push_back(static_cast<std::uint8_t>(value));
    return true;
}

bool appendU16(std::vector<std::uint8_t>& bytes, std::uint16_t value) noexcept
{
    bytes.push_back(static_cast<std::uint8_t>(value >> 8U));
    bytes.push_back(static_cast<std::uint8_t>(value));
    return true;
}

std::uint32_t readU16(const std::vector<std::uint8_t>& bytes, std::size_t offset) noexcept
{
    return static_cast<std::uint32_t>(bytes[offset]) << 8U | bytes[offset + 1];
}

std::uint32_t readU32(const std::vector<std::uint8_t>& bytes, std::size_t offset) noexcept
{
    return static_cast<std::uint32_t>(bytes[offset]) << 24U |
           static_cast<std::uint32_t>(bytes[offset + 1]) << 16U |
           static_cast<std::uint32_t>(bytes[offset + 2]) << 8U | bytes[offset + 3];
}

void appendVlq(std::vector<std::uint8_t>& bytes, std::uint32_t value) noexcept
{
    std::uint32_t buffer = value & 0x7FU;
    while ((value >>= 7U) != 0)
    {
        buffer <<= 8U;
        buffer |= ((value & 0x7FU) | 0x80U);
    }
    for (;;)
    {
        bytes.push_back(static_cast<std::uint8_t>(buffer));
        if ((buffer & 0x80U) != 0)
        {
            buffer >>= 8U;
        }
        else
        {
            break;
        }
    }
}

bool readVlq(const std::vector<std::uint8_t>& bytes, std::size_t& offset,
             std::uint32_t& value) noexcept
{
    std::uint32_t result = 0;
    for (int step = 0; step < 4; ++step)
    {
        if (offset >= bytes.size())
        {
            return false;
        }
        const std::uint8_t byte = bytes[offset++];
        result = (result << 7U) | (byte & 0x7FU);
        if ((byte & 0x80U) == 0)
        {
            value = result;
            return true;
        }
    }
    return false;
}

std::size_t dataByteCount(std::uint8_t status) noexcept
{
    switch (status & 0xF0U)
    {
    case 0xC0:
    case 0xD0:
        return 1;
    case 0x80:
    case 0x90:
    case 0xA0:
    case 0xB0:
    case 0xE0:
        return 2;
    default:
        return 0;
    }
}
} // namespace

core::Result<SmfDocument> readSmf(const std::vector<std::uint8_t>& bytes,
                                  double sampleRate) noexcept
{
    if (!std::isfinite(sampleRate) || sampleRate <= 0.0)
    {
        return core::makeError(core::ErrorCode::OutOfRange, "sample rate must be positive");
    }
    if (bytes.size() < 14 || std::memcmp(bytes.data(), "MThd", 4) != 0)
    {
        return core::makeError(core::ErrorCode::InvalidArgument, "not a Standard MIDI File");
    }
    if (readU32(bytes, 4) != 6)
    {
        return core::makeError(core::ErrorCode::InvalidArgument, "unsupported SMF header length");
    }
    const std::uint16_t format = static_cast<std::uint16_t>(readU16(bytes, 8));
    if (format > 1)
    {
        return core::makeError(core::ErrorCode::Unsupported, "only SMF format 0/1 is supported");
    }
    const std::uint16_t tracks = static_cast<std::uint16_t>(readU16(bytes, 10));
    const std::uint16_t division = static_cast<std::uint16_t>(readU16(bytes, 12));
    if (tracks == 0 || division == 0 || (division & 0x8000U) != 0)
    {
        return core::makeError(core::ErrorCode::Unsupported,
                               "only positive PPQ divisions are supported");
    }

    SmfDocument document;
    document.division = division;
    std::size_t offset = 14;
    std::uint64_t absoluteTicks = 0;
    std::uint64_t absoluteFrames = 0;
    double framesPerTick = sampleRate * 0.5 / static_cast<double>(division);
    bool foundTrack = false;

    for (std::uint16_t track = 0; track < tracks && offset + 8 <= bytes.size(); ++track)
    {
        const bool validHeader = std::memcmp(bytes.data() + offset, "MTrk", 4) == 0;
        const std::uint32_t trackBytes = readU32(bytes, offset + 4);
        offset += 8;
        if (!validHeader || trackBytes > bytes.size() - offset)
        {
            return core::makeError(core::ErrorCode::InvalidArgument, "invalid SMF track chunk");
        }
        const std::size_t end = offset + trackBytes;
        std::uint8_t runningStatus = 0;
        std::uint64_t trackTicks = 0;
        bool endOfTrack = false;

        while (offset < end && !endOfTrack)
        {
            std::uint32_t delta = 0;
            if (!readVlq(bytes, offset, delta) || offset > end)
            {
                return core::makeError(core::ErrorCode::InvalidArgument, "invalid SMF delta time");
            }
            trackTicks += delta;
            if (offset >= end)
            {
                break;
            }
            std::uint8_t status = bytes[offset];
            if ((status & 0x80U) != 0)
            {
                ++offset;
                if (status < 0xF0)
                {
                    runningStatus = status;
                }
            }
            else
            {
                status = runningStatus;
                if (status == 0)
                {
                    return core::makeError(core::ErrorCode::InvalidArgument,
                                           "SMF running status has no status byte");
                }
            }

            if (status == 0xFF)
            {
                if (offset >= end)
                {
                    return core::makeError(core::ErrorCode::InvalidArgument,
                                           "truncated SMF meta event");
                }
                const std::uint8_t type = bytes[offset++];
                std::uint32_t length = 0;
                if (!readVlq(bytes, offset, length) || offset + length > end)
                {
                    return core::makeError(core::ErrorCode::InvalidArgument,
                                           "invalid SMF meta length");
                }
                if (type == 0x51 && length == 3 && track == 0)
                {
                    document.tempoUsPerQuarter =
                        (static_cast<std::uint32_t>(bytes[offset]) << 16U) |
                        (static_cast<std::uint32_t>(bytes[offset + 1]) << 8U) | bytes[offset + 2];
                    if (document.tempoUsPerQuarter > 0)
                    {
                        framesPerTick = sampleRate *
                                        (static_cast<double>(document.tempoUsPerQuarter) /
                                         kMicrosecondsPerSecond) /
                                        static_cast<double>(division);
                    }
                }
                offset += length;
                endOfTrack = type == 0x2F;
                continue;
            }

            const std::size_t count = dataByteCount(status);
            if (offset + count > end)
            {
                return core::makeError(core::ErrorCode::InvalidArgument,
                                       "truncated SMF channel event");
            }
            std::uint8_t data1 = 0;
            std::uint8_t data2 = 0;
            if (count > 0)
            {
                data1 = bytes[offset++];
            }
            if (count > 1)
            {
                data2 = bytes[offset++];
            }
            if ((status & 0xF0U) >= 0x80 && (status & 0xF0U) <= 0xE0)
            {
                const std::uint64_t tickDelta = trackTicks - absoluteTicks;
                absoluteFrames += static_cast<std::uint64_t>(
                    std::llround(static_cast<double>(tickDelta) * framesPerTick));
                absoluteTicks = trackTicks;
                MidiEvent event;
                event.frame = absoluteFrames;
                event.status = status;
                event.data1 = data1;
                event.data2 = data2;
                document.events.push_back(event);
            }
        }
        offset = end;
        foundTrack = true;
    }

    if (!foundTrack)
    {
        return core::makeError(core::ErrorCode::InvalidArgument, "SMF contains no tracks");
    }
    std::stable_sort(document.events.begin(), document.events.end(),
                     [](const MidiEvent& left, const MidiEvent& right)
                     { return left.frame < right.frame; });
    return document;
}

core::Result<std::vector<std::uint8_t>> writeSmf(const SmfDocument& document,
                                                 double sampleRate) noexcept
{
    if (!std::isfinite(sampleRate) || sampleRate <= 0.0)
    {
        return core::makeError(core::ErrorCode::OutOfRange, "sample rate must be positive");
    }
    if (document.division == 0 || (document.division & 0x8000U) != 0 ||
        document.tempoUsPerQuarter == 0)
    {
        return core::makeError(core::ErrorCode::OutOfRange, "invalid SMF division or tempo");
    }

    std::vector<MidiEvent> events = document.events;
    std::stable_sort(events.begin(), events.end(), [](const MidiEvent& left, const MidiEvent& right)
                     { return left.frame < right.frame; });
    for (const MidiEvent& event : events)
    {
        if ((event.status & 0xF0U) < 0x80 || (event.status & 0xF0U) > 0xE0)
        {
            return core::makeError(core::ErrorCode::Unsupported,
                                   "only channel voice messages are exported");
        }
    }

    const double ticksPerFrame = static_cast<double>(document.division) * kMicrosecondsPerSecond /
                                 (sampleRate * static_cast<double>(document.tempoUsPerQuarter));
    std::vector<std::uint8_t> track;
    std::uint64_t previousFrame = 0;
    std::uint32_t previousTick = 0;

    track.push_back(0);
    track.push_back(0xFF);
    track.push_back(0x51);
    track.push_back(3);
    const std::uint32_t tempo = document.tempoUsPerQuarter;
    track.push_back(static_cast<std::uint8_t>(tempo >> 16U));
    track.push_back(static_cast<std::uint8_t>(tempo >> 8U));
    track.push_back(static_cast<std::uint8_t>(tempo));

    for (const MidiEvent& event : events)
    {
        const std::uint64_t tick =
            previousTick + static_cast<std::uint64_t>(std::llround(
                               static_cast<double>(event.frame - previousFrame) * ticksPerFrame));
        appendVlq(track, static_cast<std::uint32_t>(tick - previousTick));
        track.push_back(event.status);
        const std::size_t count = dataByteCount(event.status);
        if (count > 0)
        {
            track.push_back(static_cast<std::uint8_t>(event.data1 & 0x7FU));
        }
        if (count > 1)
        {
            track.push_back(static_cast<std::uint8_t>(event.data2 & 0x7FU));
        }
        previousFrame = event.frame;
        previousTick = static_cast<std::uint32_t>(tick);
    }

    appendVlq(track, 0);
    track.push_back(0xFF);
    track.push_back(0x2F);
    track.push_back(0);

    std::vector<std::uint8_t> bytes;
    const std::uint8_t header[] = {'M', 'T', 'h', 'd'};
    bytes.insert(bytes.end(), std::begin(header), std::end(header));
    appendU32(bytes, 6);
    appendU16(bytes, 0);
    appendU16(bytes, 1);
    appendU16(bytes, document.division);

    const std::uint8_t trackHeader[] = {'M', 'T', 'r', 'k'};
    bytes.insert(bytes.end(), std::begin(trackHeader), std::end(trackHeader));
    appendU32(bytes, static_cast<std::uint32_t>(track.size()));
    bytes.insert(bytes.end(), track.begin(), track.end());
    return bytes;
}

} // namespace axf::midi
