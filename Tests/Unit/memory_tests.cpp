#include <gtest/gtest.h>

#include "axf/core/memory.hpp"

#include <cstdint>
#include <cstring>

using namespace axf::core;

TEST(Memory, AlignedBufferIsAlignedAndClearable)
{
    AlignedBuffer buffer(1024, 64);
    ASSERT_NE(buffer.data(), nullptr);
    EXPECT_EQ(buffer.size(), 1024U);
    EXPECT_EQ(buffer.alignment(), 64U);
    EXPECT_EQ(reinterpret_cast<std::uintptr_t>(buffer.data()) % 64U, 0U);
    std::memset(buffer.data(), 0xFF, buffer.size());
    buffer.clear();
    const auto* bytes = static_cast<const unsigned char*>(buffer.data());
    for (std::size_t i = 0; i < buffer.size(); ++i)
    {
        EXPECT_EQ(bytes[i], 0U);
    }
}

TEST(Memory, PoolNeverAllocatesAfterConstruction)
{
    MonotonicPool pool(32, 4);
    EXPECT_EQ(pool.capacity(), 4U);
    EXPECT_GE(pool.blockSize(), 32U);
    void* first = pool.allocate();
    ASSERT_NE(first, nullptr);
    EXPECT_EQ(reinterpret_cast<std::uintptr_t>(first) % alignof(std::max_align_t), 0U);
    EXPECT_NE(pool.allocate(), first);
    EXPECT_NE(pool.allocate(), nullptr);
    EXPECT_NE(pool.allocate(), nullptr);
    EXPECT_EQ(pool.allocate(), nullptr);
    EXPECT_EQ(pool.available(), 0U);
    pool.reset();
    EXPECT_EQ(pool.available(), 4U);
    EXPECT_NE(pool.allocate(), nullptr);
}

TEST(Memory, TypedPoolAllocation)
{
    MonotonicPool pool(sizeof(double) * 2, 2);
    double* values = pool.allocateAs<double>();
    ASSERT_NE(values, nullptr);
    values[0] = 1.0;
    EXPECT_DOUBLE_EQ(values[0], 1.0);
}
