#include <gtest/gtest.h>

#include "axf/media/audio_file.hpp"
#include "axf/platform/platform.hpp"
#include "axf/project/project.hpp"

TEST(Platform, ReportsVersionAndOs)
{
    EXPECT_NE(std::string(axf::platform::frameworkVersion()).find("0.1.0"), std::string::npos);
    EXPECT_NE(std::string(axf::platform::osName()).empty(), true);
}

TEST(Platform, MediaExtensionDetection)
{
    EXPECT_EQ(axf::media::formatFromExtension("take.WAV"), axf::media::AudioFileFormat::Wav);
    EXPECT_EQ(axf::media::formatFromExtension("take.aiff"), axf::media::AudioFileFormat::Aiff);
    EXPECT_EQ(axf::media::formatFromExtension("take.flac"), axf::media::AudioFileFormat::Flac);
    EXPECT_EQ(axf::media::formatFromExtension("take"), axf::media::AudioFileFormat::Unknown);
    EXPECT_STREQ(axf::media::formatName(axf::media::AudioFileFormat::Wav), "wav");
}

TEST(Platform, ProjectVersionCompatibility)
{
    EXPECT_TRUE(axf::project::isCompatibleVersion(axf::core::kAxfVersion));
    EXPECT_FALSE(axf::project::isCompatibleVersion(axf::core::Version{1, 0, 0}));
}
