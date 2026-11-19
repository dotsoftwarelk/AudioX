#include <gtest/gtest.h>

#include "axf/project/automation.hpp"
#include "axf/project/autosave.hpp"
#include "axf/project/document.hpp"
#include "axf/project/undo.hpp"

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

using namespace axf;

namespace
{
core::Result<void> addTrack(project::ProjectDocument& document, const std::string& name)
{
    auto result = document.addTrack(name);
    if (!result)
    {
        return result.error();
    }
    return core::success();
}
} // namespace

TEST(ProjectControl, AutomationInterpolatesAndRoundTrips)
{
    project::ProjectDocument document;
    auto track = document.addTrack("Synth");
    ASSERT_TRUE(track.ok());

    std::vector<project::AutomationPoint> points{
        {0, 0.25F},
        {100, 1.0F},
        {200, 0.5F},
    };
    ASSERT_TRUE(
        document.setAutomation(track.value(), project::AutomationParameter::Gain, std::move(points))
            .ok());
    EXPECT_FLOAT_EQ(
        document.evaluateAutomation(track.value(), project::AutomationParameter::Gain, 0).value(),
        0.25F);
    EXPECT_FLOAT_EQ(
        document.evaluateAutomation(track.value(), project::AutomationParameter::Gain, 50).value(),
        0.625F);
    EXPECT_FLOAT_EQ(
        document.evaluateAutomation(track.value(), project::AutomationParameter::Gain, 500).value(),
        0.5F);
    EXPECT_FLOAT_EQ(
        document.evaluateAutomation(track.value(), project::AutomationParameter::Pan, 50).value(),
        0.0F);

    std::vector<project::AutomationPoint> unsorted{{10, 0.5F}, {10, 0.6F}};
    EXPECT_EQ(
        document
            .setAutomation(track.value(), project::AutomationParameter::Gain, std::move(unsorted))
            .error()
            .code,
        core::ErrorCode::InvalidArgument);
    std::vector<project::AutomationPoint> outOfRange{{0, 9.0F}};
    EXPECT_EQ(
        document
            .setAutomation(track.value(), project::AutomationParameter::Gain, std::move(outOfRange))
            .error()
            .code,
        core::ErrorCode::OutOfRange);

    const auto text = document.serialize();
    auto parsed = project::ProjectDocument::parse(text);
    ASSERT_TRUE(parsed.ok());
    ASSERT_EQ(parsed.value().automation().size(), 1U);
    EXPECT_EQ(parsed.value().automation()[0].points.size(), 3U);
    EXPECT_FLOAT_EQ(parsed.value()
                        .evaluateAutomation(track.value(), project::AutomationParameter::Gain, 50)
                        .value(),
                    0.625F);

    auto corrupt = text;
    const auto marker = corrupt.find("AUTOMATION");
    ASSERT_NE(marker, std::string::npos);
    corrupt.replace(marker, 10, "AUTOMATIOX");
    EXPECT_FALSE(project::ProjectDocument::parse(corrupt).ok());
}

TEST(ProjectControl, UndoRedoRestoresDocumentState)
{
    project::ProjectDocument document;
    project::UndoStack history(8);
    auto track = document.addTrack("Drums");
    ASSERT_TRUE(track.ok());

    ASSERT_TRUE(history
                    .apply(document, "Add bass", [](project::ProjectDocument& target)
                           { return addTrack(target, "Bass"); })
                    .ok());
    ASSERT_TRUE(history
                    .apply(document, "Set tempo",
                           [](project::ProjectDocument& target)
                           {
                               target.setTempo(140.0);
                               return core::success();
                           })
                    .ok());
    EXPECT_EQ(document.tracks().size(), 2U);
    EXPECT_DOUBLE_EQ(document.tempo(), 140.0);
    EXPECT_EQ(history.undoDescription(), "Set tempo");

    ASSERT_TRUE(history.undo(document).ok());
    EXPECT_DOUBLE_EQ(document.tempo(), 120.0);
    ASSERT_TRUE(history.undo(document).ok());
    EXPECT_EQ(document.tracks().size(), 1U);
    EXPECT_FALSE(history.canUndo());
    EXPECT_EQ(history.redoDescription(), "Add bass");

    ASSERT_TRUE(history.redo(document).ok());
    EXPECT_EQ(document.tracks().size(), 2U);
    ASSERT_TRUE(history.redo(document).ok());
    EXPECT_DOUBLE_EQ(document.tempo(), 140.0);
    EXPECT_FALSE(history.canRedo());

    history.clear();
    EXPECT_EQ(history.undoCount(), 0U);
    EXPECT_EQ(history.redoCount(), 0U);
}

TEST(ProjectControl, AutosaveRecoversLatestValidRevision)
{
    const auto directory = std::filesystem::temp_directory_path() / "axf-autosave-tests";
    std::error_code error;
    std::filesystem::remove_all(directory, error);
    std::filesystem::create_directories(directory, error);

    project::ProjectAutosave autosave(3);
    EXPECT_EQ(autosave.revisionCount(), 3U);

    project::ProjectDocument first;
    ASSERT_TRUE(addTrack(first, "First").ok());
    ASSERT_TRUE(autosave.save(first, directory, "session").ok());

    project::ProjectDocument second;
    ASSERT_TRUE(addTrack(second, "Second").ok());
    second.setTempo(96.0);
    ASSERT_TRUE(autosave.save(second, directory, "session").ok());

    auto recovered = autosave.recover(directory, "session");
    ASSERT_TRUE(recovered.ok());
    EXPECT_DOUBLE_EQ(recovered.value().tempo(), 96.0);
    ASSERT_EQ(recovered.value().tracks().size(), 1U);
    EXPECT_EQ(recovered.value().tracks()[0].name, "Second");

    const auto latest = directory / "session.autosave.2";
    {
        std::ofstream output(latest, std::ios::binary | std::ios::trunc);
        output << "corrupt";
    }
    recovered = autosave.recover(directory, "session");
    ASSERT_TRUE(recovered.ok());
    EXPECT_EQ(recovered.value().tracks()[0].name, "First");

    ASSERT_TRUE(autosave.clear(directory, "session").ok());
    EXPECT_EQ(autosave.recover(directory, "session").error().code, core::ErrorCode::NotFound);
    EXPECT_EQ(autosave.save(first, directory, "bad/name").error().code,
              core::ErrorCode::InvalidArgument);
}
