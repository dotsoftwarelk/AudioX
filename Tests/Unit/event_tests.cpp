#include <gtest/gtest.h>

#include "axf/core/event.hpp"

using namespace axf::core;

namespace
{
struct GainEvent final
{
    static constexpr events::EventTypeId typeId = 1;
    float gain{0.0F};
};

struct NoteEvent final
{
    static constexpr events::EventTypeId typeId = 2;
    std::uint8_t note{0};
    std::uint8_t velocity{0};
};

class RecordingHandler final : public events::IEventHandler
{
  public:
    void onEvent(events::EventTypeId type, const void* payload, std::size_t size,
                 std::uint64_t frame) noexcept override
    {
        ++count;
        lastType = type;
        lastFrame = frame;
        lastSize = size;
        if (type == GainEvent::typeId)
        {
            lastGain = events::asEvent<GainEvent>(payload)->gain;
        }
    }

    std::size_t count{0};
    events::EventTypeId lastType{0};
    std::uint64_t lastFrame{0};
    std::size_t lastSize{0};
    float lastGain{0.0F};
};
} // namespace

TEST(EventBus, PostsAndDispatchesTypedEvents)
{
    events::EventBus bus;
    RecordingHandler handler;
    ASSERT_TRUE(bus.addHandler(&handler).ok());
    EXPECT_EQ(bus.handlerCount(), 1U);

    EXPECT_TRUE(bus.tryPost(GainEvent{0.75F}, 48000));
    EXPECT_EQ(bus.pendingCount(), 1U);
    EXPECT_EQ(bus.dispatch(), 1U);
    EXPECT_EQ(handler.count, 1U);
    EXPECT_EQ(handler.lastType, GainEvent::typeId);
    EXPECT_EQ(handler.lastFrame, 48000U);
    EXPECT_EQ(handler.lastSize, sizeof(GainEvent));
    EXPECT_FLOAT_EQ(handler.lastGain, 0.75F);
    EXPECT_EQ(bus.pendingCount(), 0U);
}

TEST(EventBus, RejectsDuplicateHandlerAndNullHandler)
{
    events::EventBus bus;
    RecordingHandler handler;
    EXPECT_EQ(bus.addHandler(nullptr).error().code, ErrorCode::InvalidArgument);
    ASSERT_TRUE(bus.addHandler(&handler).ok());
    EXPECT_EQ(bus.addHandler(&handler).error().code, ErrorCode::AlreadyExists);
    bus.removeHandler(&handler);
    EXPECT_EQ(bus.handlerCount(), 0U);
}

TEST(EventBus, FixedCapacityIsNeverExceeded)
{
    events::EventBus bus;
    RecordingHandler handler;
    ASSERT_TRUE(bus.addHandler(&handler).ok());
    for (std::size_t i = 0; i < events::EventBus::kCapacity; ++i)
    {
        ASSERT_TRUE(bus.tryPost(NoteEvent{60, 100}, i));
    }
    EXPECT_FALSE(bus.tryPost(NoteEvent{61, 100}, 999));
    EXPECT_EQ(bus.dispatch(), events::EventBus::kCapacity);
    EXPECT_EQ(handler.count, events::EventBus::kCapacity);

    bus.clear();
    EXPECT_EQ(bus.pendingCount(), 0U);
}
