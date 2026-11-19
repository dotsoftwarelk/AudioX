#include <gtest/gtest.h>

#include "axf/media/wav_file.hpp"

#include <cmath>
#include <filesystem>
#include <vector>

using namespace axf;

namespace
{
std::filesystem::path temporaryWavPath(const char* name)
{
    return std::filesystem::temp_directory_path() / name;
}
} // namespace

TEST(WavFile, SixteenBitPcmRoundTrip)
{
    const auto path = temporaryWavPath("axf_roundtrip_stereo.wav");
    constexpr std::size_t kFrames = 256;
    std::vector<float> input(kFrames * 2);
    for (std::size_t index = 0; index < input.size(); ++index)
    {
        input[index] = static_cast<float>(std::sin(static_cast<double>(index) * 0.01) * 0.8);
    }

    media::AudioFileInfo info;
    info.format = media::AudioFileFormat::Wav;
    info.sampleRate = 48000;
    info.channels = 2;
    info.frames = kFrames;

    const auto written = media::writeWavFile(path.string(), info, input.data(), kFrames);
    ASSERT_TRUE(written.ok());
    EXPECT_EQ(written.value(), kFrames);

    media::WavReader reader;
    ASSERT_TRUE(reader.open(path.string()).ok());
    const auto readInfo = reader.info();
    ASSERT_TRUE(readInfo.ok());
    EXPECT_EQ(readInfo.value().format, media::AudioFileFormat::Wav);
    EXPECT_EQ(readInfo.value().sampleRate, 48000);
    EXPECT_EQ(readInfo.value().channels, 2);
    EXPECT_EQ(readInfo.value().frames, kFrames);

    std::vector<float> output(input.size(), 0.0F);
    const auto read = reader.read(output.data(), kFrames);
    ASSERT_TRUE(read.ok());
    EXPECT_EQ(read.value(), kFrames);
    for (std::size_t index = 0; index < input.size(); ++index)
    {
        EXPECT_NEAR(output[index], input[index], 1.0e-4F);
    }
    EXPECT_EQ(reader.read(output.data(), 8).value(), 0U);

    std::error_code error;
    std::filesystem::remove(path, error);
}

TEST(WavFile, RejectsUnsupportedFormatAndInvalidInfo)
{
    media::WavReader reader;
    EXPECT_EQ(reader.open("does-not-exist.wav").error().code, core::ErrorCode::IoError);

    media::WavWriter writer;
    media::AudioFileInfo info;
    info.format = media::AudioFileFormat::Flac;
    EXPECT_EQ(writer.open("unused.wav", info).error().code, core::ErrorCode::InvalidArgument);

    info.format = media::AudioFileFormat::Wav;
    info.channels = 0;
    EXPECT_EQ(writer.open("unused.wav", info).error().code, core::ErrorCode::OutOfRange);
}
