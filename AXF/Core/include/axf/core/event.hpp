#pragma once

#include "axf/core/export.hpp"
#include "axf/core/result.hpp"
#include "axf/core/spsc_ring_buffer.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>
#include <vector>

namespace axf::core::events
{

using EventTypeId = std::uint32_t;
constexpr std::size_t kMaxEventPayload = 64;

struct EventHeader final
{
    EventTypeId type;
    std::uint16_t payloadSize;
    std::uint64_t frame;
};

struct EventRecord final
{
    EventHeader header;
    std::byte payload[kMaxEventPayload];
};

static_assert(std::is_trivially_copyable_v<EventRecord>,
              "event records must be safe to copy on the audio thread");

/// Receives events on the control thread. Implementations must be noexcept.
class IEventHandler
{
  public:
    virtual ~IEventHandler() = default;
    virtual void onEvent(EventTypeId type, const void* payload, std::size_t payloadSize,
                         std::uint64_t frame) noexcept = 0;
};

template <class T>
constexpr bool isRtEvent = std::is_trivially_copyable_v<T> && sizeof(T) <= kMaxEventPayload;

template <class T> [[nodiscard]] const T* asEvent(const void* payload) noexcept
{
    static_assert(isRtEvent<T>, "event payload must be small and trivially copyable");
    return static_cast<const T*>(payload);
}

/// Fixed-capacity, allocation-free event posting with control-thread dispatch.
/// Any thread may post; only the control thread may register handlers or
/// dispatch. Events must be trivially copyable and expose a static `typeId`.
class EventBus final
{
  public:
    static constexpr std::size_t kCapacity = 256;

    EventBus() = default;
    EventBus(const EventBus&) = delete;
    EventBus& operator=(const EventBus&) = delete;

    [[nodiscard]] Result<void> addHandler(IEventHandler* handler);
    void removeHandler(IEventHandler* handler) noexcept;
    [[nodiscard]] std::size_t handlerCount() const noexcept
    {
        return handlers_.size();
    }

    template <class T> [[nodiscard]] bool tryPost(const T& event, std::uint64_t frame) noexcept
    {
        static_assert(requires { T::typeId; }, "event type must define static typeId");
        static_assert(isRtEvent<T>, "event must fit in the preallocated record");
        EventRecord record{};
        record.header = EventHeader{static_cast<EventTypeId>(T::typeId),
                                    static_cast<std::uint16_t>(sizeof(T)), frame};
        std::memcpy(record.payload, &event, sizeof(T));
        return queue_.tryPush(record);
    }

    [[nodiscard]] std::size_t pendingCount() const noexcept
    {
        return queue_.sizeApprox();
    }

    /// Calls every registered handler for each queued event. Returns the
    /// number of dispatched events. Control thread only.
    std::size_t dispatch();

    /// Drops queued events. Only valid when producers are stopped.
    void clear() noexcept;

  private:
    SpscRingBuffer<EventRecord, kCapacity> queue_;
    std::vector<IEventHandler*> handlers_; // control-thread owned
};

} // namespace axf::core::events
