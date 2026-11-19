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
        // Call the instrumentation API directly so this assertion is
        // independent of global operator new replacements (ASan/TSan and
        // other runtimes may provide their own definitions).
        rt::noteAllocation(sizeof(int), false);
        rt::noteRealtimeViolation();
    }
    rt::setAllocationHook(nullptr, nullptr);

    EXPECT_GE(rt::violationCount(), 2U);
    EXPECT_GE(allocations.load(), 2);
    EXPECT_FALSE(rt::isAudioThread());
}

#if defined(__SANITIZE_ADDRESS__) || defined(__SANITIZE_THREAD__)
TEST(RtAudit, GlobalOperatorNewInstrumentationSkippedUnderSanitizer)
{
    GTEST_SKIP() << "Sanitizer runtimes replace global operator new.";
}
#else
TEST(RtAudit, GlobalOperatorNewIsInstrumented)
{
    rt::resetViolations();
    std::atomic<int> allocations{0};
    rt::setAllocationHook(countAllocation, &allocations);
    {
        rt::ScopedAudioThread guard;
        void* memory = ::operator new(sizeof(int));
        ::operator delete(memory);
    }
    rt::setAllocationHook(nullptr, nullptr);

    EXPECT_GE(rt::violationCount(), 1U);
    EXPECT_GE(allocations.load(), 2);
}
#endif

TEST(RtAudit, ControlThreadAllocationIsIgnored)
{
    rt::resetViolations();
    std::atomic<int> allocations{0};
    rt::setAllocationHook(countAllocation, &allocations);
    void* memory = ::operator new(sizeof(int));
    ::operator delete(memory);
    rt::setAllocationHook(nullptr, nullptr);
    EXPECT_EQ(rt::violationCount(), 0U);
}
