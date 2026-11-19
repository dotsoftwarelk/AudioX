#pragma once

#include "axf/core/result.hpp"
#include "axf/core/types.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

namespace axf::media
{

enum class AudioFileFormat : std::uint8_t
{
    Unknown,
    Wav,
    Aiff,
    Flac
};

[[nodiscard]] AXF_API const char* formatName(AudioFileFormat format) noexcept;
[[nodiscard]] AXF_API AudioFileFormat formatFromExtension(const std::string& path) noexcept;

struct AudioFileInfo final
{
    AudioFileFormat format{AudioFileFormat::Unknown};
    core::SampleRate sampleRate{0};
    core::ChannelCount channels{0};
    std::uint64_t frames{0};
};

class IAudioFileReader
{
  public:
    virtual ~IAudioFileReader() = default;
    [[nodiscard]] virtual core::Result<AudioFileInfo> info() const = 0;
    /// Reads interleaved floats; returns the number of frames actually read.
    [[nodiscard]] virtual core::Result<std::size_t> read(float* interleaved,
                                                         std::size_t frames) = 0;
};

class IAudioFileWriter
{
  public:
    virtual ~IAudioFileWriter() = default;
    [[nodiscard]] virtual core::Result<void> open(const std::string& path,
                                                  const AudioFileInfo& info) = 0;
    [[nodiscard]] virtual core::Result<void> write(const float* interleaved,
                                                   std::size_t frames) = 0;
    [[nodiscard]] virtual core::Result<void> close() = 0;
};

} // namespace axf::media
