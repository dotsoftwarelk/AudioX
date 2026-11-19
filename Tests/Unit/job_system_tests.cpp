#include <gtest/gtest.h>

#include "axf/core/job_system.hpp"

#include <atomic>

using namespace axf::core;

TEST(JobSystem, ExecutesAllSubmittedJobs)
{
    JobSystem jobs(2);
    ASSERT_EQ(jobs.workerCount(), 2U);
    std::atomic<int> counter{0};
    for (int i = 0; i < 100; ++i)
    {
        jobs.submit([&counter] { counter.fetch_add(1, std::memory_order_relaxed); });
    }
    jobs.waitForAll();
    EXPECT_EQ(counter.load(), 100);
    EXPECT_EQ(jobs.pendingCount(), 0U);
}

TEST(JobSystem, AcceptsPriorityAndIgnoresEmptyJobs)
{
    JobSystem jobs(1);
    std::atomic<int> high{0};
    std::atomic<int> low{0};
    jobs.submit([&high] { high.fetch_add(1, std::memory_order_relaxed); }, JobPriority::High);
    jobs.submit([&low] { low.fetch_add(1, std::memory_order_relaxed); }, JobPriority::Low);
    jobs.submit(std::function<void()>{});
    jobs.waitForAll();
    EXPECT_EQ(high.load(), 1);
    EXPECT_EQ(low.load(), 1);
}

TEST(JobSystem, ShutdownIsIdempotent)
{
    JobSystem jobs(1);
    EXPECT_FALSE(jobs.isShutdown());
    jobs.shutdown();
    EXPECT_TRUE(jobs.isShutdown());
    jobs.shutdown();
    std::atomic<int> counter{0};
    jobs.submit([&counter] { counter.fetch_add(1, std::memory_order_relaxed); });
    jobs.waitForAll();
    EXPECT_EQ(counter.load(), 0);
}
