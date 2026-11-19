#include <gtest/gtest.h>

#include "axf/midi/smf.hpp"

#include <cstdint>
#include <vector>

using namespace axf;

TEST(SmfFile, FormatZeroRoundTrip)
{
    midi::SmfDocument document;
    document.division = 960;
    document.tempoUsPerQuarter = 500000;
    document.events.push_back(midi::makeNoteOn(0, 0, 60, 100));
    document.events.push_back(midi::makeNoteOff(25, 0, 60, 0));
    document.events.push_back(midi::makeControlChange(2500, 2, 7, 64));
    document.events.push_back(midi::makeNoteOn(5000, 1, 64, 90));
    document.events.push_back(midi::makeNoteOff(10000, 1, 64, 0));

    const auto bytes = midi::writeSmf(document, 48000.0);
    ASSERT_TRUE(bytes.ok());
    ASSERT_GT(bytes.value().size(), 14U);
    EXPECT_EQ(bytes.value()[0], 'M');
    EXPECT_EQ(bytes.value()[8], 0);
    EXPECT_EQ(bytes.value()[11], 1);

    const auto parsed = midi::readSmf(bytes.value(), 48000.0);
    ASSERT_TRUE(parsed.ok());
    EXPECT_EQ(parsed.value().division, 960);
    EXPECT_EQ(parsed.value().tempoUsPerQuarter, 500000U);
    ASSERT_EQ(parsed.value().events.size(), document.events.size());
    for (std::size_t index = 0; index < document.events.size(); ++index)
    {
        EXPECT_EQ(parsed.value().events[index].frame, document.events[index].frame);
        EXPECT_EQ(parsed.value().events[index].status, document.events[index].status);
        EXPECT_EQ(parsed.value().events[index].data1, document.events[index].data1);
        EXPECT_EQ(parsed.value().events[index].data2, document.events[index].data2);
    }
}

TEST(SmfFile, RejectsInvalidInput)
{
    EXPECT_EQ(midi::readSmf({}, 48000.0).error().code, core::ErrorCode::InvalidArgument);
    EXPECT_EQ(midi::readSmf({1, 2, 3}, 48000.0).error().code, core::ErrorCode::InvalidArgument);
    EXPECT_EQ(midi::readSmf({}, 0.0).error().code, core::ErrorCode::OutOfRange);

    midi::SmfDocument document;
    document.division = 0;
    EXPECT_EQ(midi::writeSmf(document, 48000.0).error().code, core::ErrorCode::OutOfRange);

    document = midi::SmfDocument{};
    document.events.push_back({0, 0xF0, 0, 0});
    EXPECT_EQ(midi::writeSmf(document, 48000.0).error().code, core::ErrorCode::Unsupported);
}
