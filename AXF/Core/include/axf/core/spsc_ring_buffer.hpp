#pragma once

#include "axf/core/export.hpp"
#include "axf/core/types.hpp"

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace axf::core
{

/// Single-producer / single-consumer lock-free queue with fixed capacity.
/// Exactly one producer calls tryPush and exactly one consumer calls tryPop.
/// No allocation, no locking, no system calls in either path.
template <class T, std::size_t Capacity> class SpscRingBuffer final
{
    static_assert(Capacity > 0, "Capacity must be non-zero");
    static_assert(isPowerOfTwo(Capacity), "Capacity must be a power of two");
    static_assert(std::is_trivially_copyable_v<T>, "RT payload must be trivially copyable");
    static_assert(std::is_trivially_default_constructible_v<T>,
                  "RT payload must be trivially default constructible");

  public:
    using value_type = T;

    SpscRingBuffer() = default;
    SpscRingBuffer(const SpscRingBuffer&) = delete;
    SpscRingBuffer& operator=(const SpscRingBuffer&) = delete;

    [[nodiscard]] bool tryPush(const T& item) noexcept
    {
        const Index tail = tail_.load(std::memory_order_relaxed);
        const Index head = head_.load(std::memory_order_acquire);
        if (tail - head >= static_cast<Index>(Capacity))
        {
            return false;
        }
        slots_[tail & kMask] = item;
        tail_.store(tail + 1, std::memory_order_release);
        return true;
    }

    [[nodiscard]] bool tryPop(T& item) noexcept
    {
        const Index head = head_.load(std::memory_order_relaxed);
        const Index tail = tail_.load(std::memory_order_acquire);
        if (head == tail)
        {
            return false;
        }
        item = slots_[head & kMask];
        head_.store(head + 1, std::memory_order_release);
        return true;
    }

    /// Approximate because producer and consumer advance independently.
    [[nodiscard]] std::size_t sizeApprox() const noexcept
    {
        const Index tail = tail_.load(std::memory_order_relaxed);
        const Index head = head_.load(std::memory_order_relaxed);
        return tail > head ? static_cast<std::size_t>(tail - head) : 0U;
    }

    [[nodiscard]] bool empty() const noexcept
    {
        return sizeApprox() == 0;
    }

    /// Only valid when no producer/consumer is active.
    void reset() noexcept
    {
        head_.store(0, std::memory_order_relaxed);
        tail_.store(0, std::memory_order_relaxed);
    }

  private:
    using Index = std::uint64_t;
    static constexpr Index kMask = static_cast<Index>(Capacity - 1);

    alignas(64) std::atomic<Index> head_{0};
    alignas(64) std::atomic<Index> tail_{0};
    std::array<T, Capacity> slots_{};
};

} // namespace axf::core
