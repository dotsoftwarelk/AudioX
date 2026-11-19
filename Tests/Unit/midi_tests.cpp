#include <gtest/gtest.h>

#include "axf/midi/event.hpp"

#include <array>

using namespace axf::midi;

TEST(MidiEvent, NoteOnEncoding)
{
    const MidiEvent event = makeNoteOn(44100, 3, 60, 100);
    EXPECT_EQ(event.frame, 44100U);
    EXPECT_EQ(messageType(event), MidiMessageType::NoteOn);
    EXPECT_EQ(eventChannel(event), 3);
    EXPECT_EQ(event.data1, 60);
    EXPECT_EQ(event.data2, 100);
}

TEST(MidiEvent, ControlChangeEncoding)
{
    const MidiEvent event = makeControlChange(0, 15, 7, 127);
    EXPECT_EQ(messageType(event), MidiMessageType::ControlChange);
    EXPECT_EQ(eventChannel(event), 15);
    EXPECT_EQ(event.data1, 7);
    EXPECT_EQ(event.data2, 127);
}

TEST(MidiEvent, BufferNeverOverflows)
{
    std::array<MidiEvent, 2> storage{};
    MidiEventBufferView view{storage.data(), 0, storage.size()};
    EXPECT_TRUE(view.push(makeNoteOff(0, 0, 60, 0)));
    EXPECT_TRUE(view.push(makeNoteOn(1, 0, 61, 90)));
    EXPECT_FALSE(view.push(makeNoteOn(2, 0, 62, 90)));
    EXPECT_EQ(view.size, 2U);
    view.clear();
    EXPECT_EQ(view.size, 0U);
}
