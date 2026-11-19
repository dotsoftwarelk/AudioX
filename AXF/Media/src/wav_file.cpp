#include "axf/media/wav_file.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <fstream>
#include <limits>
#include <vector>

namespace axf::media
{
namespace
{
constexpr std::uint16_t kPcmFormat = 1;

void writeLittleEndian16(std::uint8_t* destination, std::uint16_t value) noexcept
{
    destination[0] = static_cast<std::uint8_t>(value & 0xFFU);
    destination[1] = static_cast<std::uint8_t>(value >> 8U);
}

void writeLittleEndian32(std::uint8_t* destination, std::uint32_t value) noexcept
{
    for (std::size_t byte = 0; byte < 4; ++byte)
    {
        destination[byte] = static_cast<std::uint8_t>(value >> (8U * byte));
    }
}

std::uint16_t readLittleEndian16(const std::uint8_t* source) noexcept
{
    return static_cast<std::uint16_t>(static_cast<std::uint16_t>(source[0]) |
                                      static_cast<std::uint16_t>(source[1] << 8U));
}

std::uint32_t readLittleEndian32(const std::uint8_t* source) noexcept
{
    std::uint32_t result = 0;
    for (std::size_t byte = 0; byte < 4; ++byte)
    {
        result |= static_cast<std::uint32_t>(source[byte]) << (8U * byte);
    }
    return result;
}

std::int16_t floatToPcm16(float value) noexcept
{
    const float clamped = std::clamp(value, -1.0F, 1.0F);
    if (clamped >= 0.0F)
    {
        return static_cast<std::int16_t>(clamped * 32767.0F);
    }
    return static_cast<std::int16_t>(clamped * 32768.0F);
}

float pcm16ToFloat(std::int16_t value) noexcept
{
    return value >= 0 ? static_cast<float>(value) / 32767.0F : static_cast<float>(value) / 32768.0F;
}

core::Result<void> validInfo(const AudioFileInfo& info) noexcept
{
    if (info.format != AudioFileFormat::Wav)
    {
        return core::makeError(core::ErrorCode::InvalidArgument, "only WAV output is implemented");
    }
    if (!core::isValidSampleRate(info.sampleRate) || info.channels == 0 || info.channels > 512)
    {
        return core::makeError(core::ErrorCode::OutOfRange,
                               "invalid WAV sample rate or channel count");
    }
    return core::success();
}
} // namespace

core::Result<void> WavReader::open(const std::string& path) noexcept
{
    std::ifstream stream(path, std::ios::binary);
    if (!stream)
    {
        return core::makeError(core::ErrorCode::IoError, "cannot open WAV file");
    }
    std::array<std::uint8_t, 12> riff{};
    stream.read(reinterpret_cast<char*>(riff.data()), static_cast<std::streamsize>(riff.size()));
    if (stream.gcount() != static_cast<std::streamsize>(riff.size()) ||
        std::memcmp(riff.data(), "RIFF", 4) != 0 || std::memcmp(riff.data() + 8, "WAVE", 4) != 0)
    {
        return core::makeError(core::ErrorCode::InvalidArgument, "not a RIFF/WAVE file");
    }

    bool haveFormat = false;
    bool haveData = false;
    std::uint32_t sampleRate = 0;
    std::uint16_t channels = 0;
    std::uint16_t bitsPerSample = 0;

    while (!haveData)
    {
        std::array<std::uint8_t, 8> header{};
        stream.read(reinterpret_cast<char*>(header.data()), 8);
        if (stream.gcount() != 8)
        {
            return core::makeError(core::ErrorCode::InvalidArgument, "truncated WAV chunk");
        }
        const std::uint32_t size = readLittleEndian32(header.data() + 4);
        if (std::memcmp(header.data(), "fmt ", 4) == 0)
        {
            if (size < 16)
            {
                return core::makeError(core::ErrorCode::InvalidArgument, "short WAV format chunk");
            }
            std::array<std::uint8_t, 16> format{};
            stream.read(reinterpret_cast<char*>(format.data()), 16);
            if (stream.gcount() != 16)
            {
                return core::makeError(core::ErrorCode::InvalidArgument, "truncated WAV format");
            }
            const std::uint16_t audioFormat = readLittleEndian16(format.data());
            channels = readLittleEndian16(format.data() + 2);
            sampleRate = readLittleEndian32(format.data() + 4);
            bitsPerSample = readLittleEndian16(format.data() + 14);
            if (audioFormat != kPcmFormat || bitsPerSample != 16 || channels == 0 ||
                !core::isValidSampleRate(sampleRate))
            {
                return core::makeError(core::ErrorCode::Unsupported,
                                       "only 16-bit PCM WAV is supported");
            }
            haveFormat = true;
            if (size > 16)
            {
                stream.seekg(static_cast<std::streamoff>(size - 16), std::ios::cur);
            }
        }
        else if (std::memcmp(header.data(), "data", 4) == 0)
        {
            if (!haveFormat)
            {
                return core::makeError(core::ErrorCode::InvalidArgument,
                                       "WAV data chunk precedes format");
            }
            dataOffset_ = static_cast<std::uint64_t>(stream.tellg());
            blockAlign_ = static_cast<std::uint16_t>(channels * 2U);
            info_ = AudioFileInfo{AudioFileFormat::Wav, static_cast<core::SampleRate>(sampleRate),
                                  static_cast<core::ChannelCount>(channels), size / blockAlign_};
            framesRead_ = 0;
            haveData = true;
        }
        else
        {
            stream.seekg(static_cast<std::streamoff>(size + (size & 1U)), std::ios::cur);
        }
    }

    path_ = path;
    return core::success();
}

core::Result<AudioFileInfo> WavReader::info() const noexcept
{
    if (path_.empty())
    {
        return core::makeError(core::ErrorCode::NotFound, "reader is not open");
    }
    return info_;
}

core::Result<std::size_t> WavReader::read(float* interleaved, std::size_t frames) noexcept
{
    if (path_.empty() || interleaved == nullptr)
    {
        return core::makeError(core::ErrorCode::InvalidArgument,
                               "reader is not open or output is null");
    }
    const std::size_t remaining =
        static_cast<std::size_t>(std::max<std::uint64_t>(0, info_.frames - framesRead_));
    const std::size_t count = std::min(frames, remaining);
    if (count == 0)
    {
        return 0U;
    }

    std::ifstream stream(path_, std::ios::binary);
    if (!stream)
    {
        return core::makeError(core::ErrorCode::IoError, "cannot reopen WAV file");
    }
    stream.seekg(static_cast<std::streamoff>(dataOffset_ + framesRead_ * blockAlign_),
                 std::ios::beg);
    std::vector<std::uint8_t> bytes(count * blockAlign_);
    stream.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    if (stream.gcount() != static_cast<std::streamsize>(bytes.size()))
    {
        return core::makeError(core::ErrorCode::IoError, "truncated WAV data");
    }
    for (std::size_t sample = 0; sample < count * info_.channels; ++sample)
    {
        const auto value = static_cast<std::int16_t>(readLittleEndian16(bytes.data() + sample * 2));
        interleaved[sample] = pcm16ToFloat(value);
    }
    framesRead_ += count;
    return count;
}

WavWriter::~WavWriter()
{
    if (open_)
    {
        (void)close();
    }
}

core::Result<void> WavWriter::open(const std::string& path, const AudioFileInfo& info) noexcept
{
    const auto validation = validInfo(info);
    if (!validation)
    {
        return validation.error();
    }
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    if (!stream)
    {
        return core::makeError(core::ErrorCode::IoError, "cannot create WAV file");
    }
    std::array<std::uint8_t, 44> header{};
    std::memcpy(header.data(), "RIFF", 4);
    writeLittleEndian32(header.data() + 4, 36);
    std::memcpy(header.data() + 8, "WAVE", 4);
    std::memcpy(header.data() + 12, "fmt ", 4);
    writeLittleEndian32(header.data() + 16, 16);
    writeLittleEndian16(header.data() + 20, kPcmFormat);
    writeLittleEndian16(header.data() + 22, info.channels);
    writeLittleEndian32(header.data() + 24, static_cast<std::uint32_t>(info.sampleRate));
    const std::uint16_t blockAlign = static_cast<std::uint16_t>(info.channels * 2U);
    const std::uint32_t byteRate = static_cast<std::uint32_t>(info.sampleRate) * blockAlign;
    writeLittleEndian32(header.data() + 28, byteRate);
    writeLittleEndian16(header.data() + 32, blockAlign);
    writeLittleEndian16(header.data() + 34, 16);
    std::memcpy(header.data() + 36, "data", 4);
    stream.write(reinterpret_cast<const char*>(header.data()),
                 static_cast<std::streamsize>(header.size()));
    if (!stream)
    {
        return core::makeError(core::ErrorCode::IoError, "cannot write WAV header");
    }
    path_ = path;
    info_ = info;
    info_.frames = 0;
    framesWritten_ = 0;
    open_ = true;
    return core::success();
}

core::Result<void> WavWriter::write(const float* interleaved, std::size_t frames) noexcept
{
    if (!open_ || interleaved == nullptr)
    {
        return core::makeError(core::ErrorCode::InvalidArgument,
                               "writer is not open or input is null");
    }
    std::ofstream stream(path_, std::ios::binary | std::ios::app);
    if (!stream)
    {
        return core::makeError(core::ErrorCode::IoError, "cannot append WAV data");
    }
    std::vector<std::uint8_t> bytes(frames * info_.channels * 2U);
    for (std::size_t sample = 0; sample < frames * info_.channels; ++sample)
    {
        const std::uint16_t pcm = static_cast<std::uint16_t>(
            static_cast<std::uint16_t>(floatToPcm16(interleaved[sample])) & 0xFFFFU);
        writeLittleEndian16(bytes.data() + sample * 2, pcm);
    }
    stream.write(reinterpret_cast<const char*>(bytes.data()),
                 static_cast<std::streamsize>(bytes.size()));
    if (!stream)
    {
        return core::makeError(core::ErrorCode::IoError, "cannot write WAV data");
    }
    framesWritten_ += frames;
    return core::success();
}

core::Result<void> WavWriter::close() noexcept
{
    if (!open_)
    {
        return core::success();
    }
    const std::uint32_t dataBytes =
        static_cast<std::uint32_t>(framesWritten_ * info_.channels * 2U);
    std::fstream stream(path_, std::ios::binary | std::ios::in | std::ios::out);
    if (!stream)
    {
        return core::makeError(core::ErrorCode::IoError, "cannot reopen WAV for finalisation");
    }
    std::array<std::uint8_t, 4> size{};
    writeLittleEndian32(size.data(), 36U + dataBytes);
    stream.seekp(4, std::ios::beg);
    stream.write(reinterpret_cast<const char*>(size.data()), 4);
    writeLittleEndian32(size.data(), dataBytes);
    stream.seekp(40, std::ios::beg);
    stream.write(reinterpret_cast<const char*>(size.data()), 4);
    stream.close();
    open_ = false;
    info_.frames = framesWritten_;
    return core::success();
}

bool WavWriter::isOpen() const noexcept
{
    return open_;
}

std::uint64_t WavWriter::framesWritten() const noexcept
{
    return framesWritten_;
}

core::Result<std::size_t> writeWavFile(const std::string& path, const AudioFileInfo& info,
                                       const float* interleaved, std::size_t frames) noexcept
{
    WavWriter writer;
    auto result = writer.open(path, info);
    if (!result)
    {
        return result.error();
    }
    result = writer.write(interleaved, frames);
    if (!result)
    {
        return result.error();
    }
    result = writer.close();
    if (!result)
    {
        return result.error();
    }
    return frames;
}

} // namespace axf::media
