#pragma once

#include "axf/media/audio_file.hpp"

#include <cstdint>
#include <string>

namespace axf::media
{

/// Minimal RIFF/WAVE reader for 16-bit PCM. File I/O is control-thread only.
class WavReader final : public IAudioFileReader
{
  public:
    WavReader() = default;
    ~WavReader() override = default;
    WavReader(const WavReader&) = delete;
    WavReader& operator=(const WavReader&) = delete;

    [[nodiscard]] core::Result<void> open(const std::string& path) noexcept;
    [[nodiscard]] core::Result<AudioFileInfo> info() const noexcept override;
    [[nodiscard]] core::Result<std::size_t> read(float* interleaved,
                                                 std::size_t frames) noexcept override;

  private:
    AudioFileInfo info_{};
    std::string path_;
    std::uint64_t dataOffset_{0};
    std::uint64_t framesRead_{0};
    std::uint16_t blockAlign_{0};
};

/// Writes 16-bit PCM WAV files. open/write/close are control-thread operations.
class WavWriter final : public IAudioFileWriter
{
  public:
    WavWriter() = default;
    ~WavWriter() override;
    WavWriter(const WavWriter&) = delete;
    WavWriter& operator=(const WavWriter&) = delete;

    [[nodiscard]] core::Result<void> open(const std::string& path,
                                          const AudioFileInfo& info) noexcept override;
    [[nodiscard]] core::Result<void> write(const float* interleaved,
                                           std::size_t frames) noexcept override;
    [[nodiscard]] core::Result<void> close() noexcept override;
    [[nodiscard]] bool isOpen() const noexcept;
    [[nodiscard]] std::uint64_t framesWritten() const noexcept;

  private:
    std::string path_;
    AudioFileInfo info_{};
    std::uint64_t framesWritten_{0};
    bool open_{false};
};

[[nodiscard]] core::Result<std::size_t> writeWavFile(const std::string& path,
                                                     const AudioFileInfo& info,
                                                     const float* interleaved,
                                                     std::size_t frames) noexcept;

} // namespace axf::media
