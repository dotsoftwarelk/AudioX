#include "axf/core/event.hpp"

#include <algorithm>

namespace axf::core::events
{

Result<void> EventBus::addHandler(IEventHandler* handler)
{
    if (handler == nullptr)
    {
        return makeError(ErrorCode::InvalidArgument, "event handler is null");
    }
    if (std::find(handlers_.begin(), handlers_.end(), handler) != handlers_.end())
    {
        return makeError(ErrorCode::AlreadyExists, "event handler is already registered");
    }
    handlers_.push_back(handler);
    return success();
}

void EventBus::removeHandler(IEventHandler* handler) noexcept
{
    handlers_.erase(std::remove(handlers_.begin(), handlers_.end(), handler), handlers_.end());
}

std::size_t EventBus::dispatch()
{
    EventRecord record{};
    std::size_t dispatched = 0;
    while (queue_.tryPop(record))
    {
        for (IEventHandler* handler : handlers_)
        {
            handler->onEvent(record.header.type, record.payload, record.header.payloadSize,
                             record.header.frame);
        }
        ++dispatched;
    }
    return dispatched;
}

void EventBus::clear() noexcept
{
    queue_.reset();
}

} // namespace axf::core::events
