#include <gtest/gtest.h>

#include "axf/project/document.hpp"

#include <string>

using namespace axf;

TEST(ProjectDocument, TracksClipsAndSettingsRoundTrip)
{
    project::ProjectDocument document;
    document.setName("Demo\tTake");
    document.setTempo(96.5);
    const auto drums = document.addTrack("Drums");

    ASSERT_TRUE(drums.ok());
    ASSERT_TRUE(document.setTrackGain(drums.value(), 0.8F).ok());
    ASSERT_TRUE(document.setTrackPan(drums.value(), -0.25F).ok());
    ASSERT_TRUE(document.setTrackMute(drums.value(), true).ok());
    ASSERT_TRUE(document.setTrackSolo(drums.value(), false).ok());
    const auto clip = document.addClip(drums.value(), "take\\one.wav", 48000, 96000);
    ASSERT_TRUE(clip.ok());

    const std::string text = document.serialize();
    EXPECT_NE(text.find("AXF-PROJECT"), std::string::npos);
    EXPECT_NE(text.find("Demo\\tTake"), std::string::npos);

    auto parsed = project::ProjectDocument::parse(text);
    ASSERT_TRUE(parsed.ok());
    EXPECT_EQ(parsed.value().name(), "Demo\tTake");
    EXPECT_DOUBLE_EQ(parsed.value().tempo(), 96.5);
    ASSERT_EQ(parsed.value().tracks().size(), 1U);
    const auto& track = parsed.value().tracks()[0];
    EXPECT_EQ(track.id, drums.value());
    EXPECT_FLOAT_EQ(track.gain, 0.8F);
    EXPECT_FLOAT_EQ(track.pan, -0.25F);
    EXPECT_TRUE(track.muted);
    EXPECT_FALSE(track.soloed);
    ASSERT_EQ(track.clips.size(), 1U);
    EXPECT_EQ(track.clips[0].id, clip.value());
    EXPECT_EQ(track.clips[0].startFrame, 48000U);
    EXPECT_EQ(track.clips[0].lengthFrames, 96000U);
    EXPECT_EQ(track.clips[0].source, "take\\one.wav");

    const auto next = parsed.value().addTrack("Bass");
    ASSERT_TRUE(next.ok());
    EXPECT_GT(next.value(), clip.value());
}

TEST(ProjectDocument, RejectsInvalidOperations)
{
    project::ProjectDocument document;
    EXPECT_EQ(document.removeTrack(99).error().code, core::ErrorCode::NotFound);
    EXPECT_EQ(document.setTrackGain(99, 0.5F).error().code, core::ErrorCode::NotFound);
    EXPECT_EQ(document.addClip(99, "x.wav", 0, 10).error().code, core::ErrorCode::NotFound);

    const auto track = document.addTrack("Track");
    ASSERT_TRUE(track.ok());
    EXPECT_EQ(document.setTrackGain(track.value(), 5.0F).error().code, core::ErrorCode::OutOfRange);
    EXPECT_EQ(document.setTrackPan(track.value(), -2.0F).error().code, core::ErrorCode::OutOfRange);
    EXPECT_EQ(document.addClip(track.value(), "x.wav", 0, 0).error().code,
              core::ErrorCode::OutOfRange);

    const auto clip = document.addClip(track.value(), "x.wav", 100, 50);
    ASSERT_TRUE(clip.ok());
    EXPECT_TRUE(document.removeClip(clip.value()).ok());
    EXPECT_EQ(document.removeClip(clip.value()).error().code, core::ErrorCode::NotFound);
    EXPECT_TRUE(document.removeTrack(track.value()).ok());
}

TEST(ProjectDocument, RejectsCorruptSerialisation)
{
    EXPECT_EQ(project::ProjectDocument::parse("").error().code, core::ErrorCode::InvalidArgument);
    EXPECT_EQ(project::ProjectDocument::parse("BAD\t0.1\nEND\n").error().code,
              core::ErrorCode::InvalidArgument);
    EXPECT_EQ(project::ProjectDocument::parse("AXF-PROJECT\t0.1\nNAME\tX\n").error().code,
              core::ErrorCode::InvalidArgument);
    EXPECT_EQ(project::ProjectDocument::parse("AXF-PROJECT\t0.1\nNAME\tX\nTEMPO\t1000\nEND\n")
                  .error()
                  .code,
              core::ErrorCode::OutOfRange);
}
