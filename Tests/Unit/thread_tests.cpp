#include <gtest/gtest.h>

#include "axf/core/thread.hpp"

using namespace axf::core;

TEST(ThreadUtilities, IdentityAndNames)
{
    EXPECT_NE(thread::currentThreadId(), 0U);
    thread::setCurrentThreadName("axf-test");
    EXPECT_STREQ(thread::threadPriorityName(thread::ThreadPriority::Low), "low");
    EXPECT_STREQ(thread::threadPriorityName(thread::ThreadPriority::Normal), "normal");
    EXPECT_STREQ(thread::threadPriorityName(thread::ThreadPriority::High), "high");
}

TEST(ThreadUtilities, NormalPriorityIsPortable)
{
    EXPECT_TRUE(thread::setCurrentThreadPriority(thread::ThreadPriority::Normal).ok());
}
