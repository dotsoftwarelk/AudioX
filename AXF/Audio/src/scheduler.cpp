#include "axf/audio/scheduler.hpp"

#include <algorithm>
#include <cstring>

namespace axf::audio
{

EventScheduler::EventScheduler(std::size_t capacity)
    : events_(capacity == 0 ? nullptr : std::make_unique<ScheduledEvent[]>(capacity)),
      capacity_(capacity)
{
}

bool EventScheduler::trySchedule(std::uint64_t frame, core::events::EventTypeId type,
                                 const void* payload, std::size_t payloadSize) noexcept
{
    if (payloadSize > sizeof(ScheduledEvent{}.payload) || count_ >= capacity_)
    {
        return false;
    }

    ScheduledEvent event{};
    event.frame = frame;
    event.sequence = nextSequence_++;
    event.type = type;
    event.payloadSize = static_cast<std::uint16_t>(payloadSize);
    if (payloadSize > 0 && payload != nullptr)
    {
        std::memcpy(event.payload, payload, payloadSize);
    }

    std::size_t position = count_;
    while (position > 0)
    {
        const ScheduledEvent& previous = events_[position - 1];
        if (previous.frame < event.frame ||
            (previous.frame == event.frame && previous.sequence < event.sequence))
        {
            break;
        }
        events_[position] = events_[position - 1];
        --position;
    }
    events_[position] = event;
    ++count_;
    return true;
}

bool EventScheduler::popDue(std::uint64_t frame, ScheduledEvent& event) noexcept
{
    if (count_ == 0 || events_[0].frame > frame)
    {
        return false;
    }
    event = events_[0];
    for (std::size_t i = 1; i < count_; ++i)
    {
        events_[i - 1] = events_[i];
    }
    --count_;
    return true;
}

} // namespace axf::audio
