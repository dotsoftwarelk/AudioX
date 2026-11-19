#include <gtest/gtest.h>

#include "axf/core/rt.hpp"

#include <atomic>

using namespace axf::core;

namespace
{
void countAllocation(std::size_t, bool, void* userData) noexcept
{
    static_cast<std::atomic<int>*>(userData)->fetch_add(1, std::memory_order_relaxed);
}
} // namespace

TEST(RtAudit, DetectsAllocationOnMarkedAudioThread)
{
    rt::resetViolations();
    std::atomic<int> allocations{0};
    rt::setAllocationHook(countAllocation, &allocations);
    {
        rt::ScopedAudioThread guard;
        int* value = new int(42);
        EXPECT_EQ(*value, 42);
        delete value;
        rt::noteRealtimeViolation();
    }
    rt::setAllocationHook(nullptr, nullptr);

    EXPECT_EQ(rt::violationCount(), 2U);
    EXPECT_GE(allocations.load(), 1);
    EXPECT_FALSE(rt::isAudioThread());
}

TEST(RtAudit, ControlThreadAllocationIsIgnored)
{
    rt::resetViolations();
    std::atomic<int> allocations{0};
    rt::setAllocationHook(countAllocation, &allocations);
    int* value = new int(7);
    delete value;
    rt::setAllocationHook(nullptr, nullptr);
    EXPECT_EQ(rt::violationCount(), 0U);
}
