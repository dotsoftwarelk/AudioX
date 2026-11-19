#pragma once

#include "axf/core/event.hpp"
#include "axf/core/export.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>

namespace axf::audio
{

struct ScheduledEvent final
{
    std::uint64_t frame;
    std::uint32_t sequence;
    core::events::EventTypeId type;
    std::uint16_t payloadSize;
    // naturally 8-byte aligned, avoiding MSVC C4324 from an explicit alignas
    std::uint64_t payload[4];
};

/// Fixed-capacity, allocation-free sample-accurate event queue. Insertion and
/// removal are bounded linear scans; capacity is chosen by the host.
class EventScheduler final
{
  public:
    explicit EventScheduler(std::size_t capacity);

    EventScheduler(const EventScheduler&) = delete;
    EventScheduler& operator=(const EventScheduler&) = delete;

    [[nodiscard]] bool trySchedule(std::uint64_t frame, core::events::EventTypeId type,
                                   const void* payload, std::size_t payloadSize) noexcept;

    [[nodiscard]] bool popDue(std::uint64_t frame, ScheduledEvent& event) noexcept;

    [[nodiscard]] std::size_t count() const noexcept
    {
        return count_;
    }
    [[nodiscard]] std::size_t capacity() const noexcept
    {
        return capacity_;
    }

    void clear() noexcept
    {
        count_ = 0;
    }

  private:
    std::unique_ptr<ScheduledEvent[]> events_;
    std::size_t capacity_;
    std::size_t count_{0};
    std::uint32_t nextSequence_{0};
};

} // namespace axf::audio
