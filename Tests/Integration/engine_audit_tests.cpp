#include <gtest/gtest.h>

#include "axf/audio/engine.hpp"
#include "axf/core/rt.hpp"

#include <array>
#include <atomic>

using namespace axf;
using namespace axf::core;

namespace
{
void countAllocation(std::size_t, bool, void* userData) noexcept
{
    static_cast<std::atomic<int>*>(userData)->fetch_add(1, std::memory_order_relaxed);
}
} // namespace

TEST(EngineAudit, AudioCallbackDoesNotAllocate)
{
    graph::Graph description;
    const auto input = description.addNode(graph::NodeKind::AudioInput, "in", 0, 1);
    const auto gain = description.addNode(graph::NodeKind::Gain, "gain", 1, 1);
    const auto output = description.addNode(graph::NodeKind::AudioOutput, "out", 1, 0);
    ASSERT_TRUE(input && gain && output);
    ASSERT_TRUE(
        description.connect({input.value(), 0}, {gain.value(), 0}, graph::MediaType::Audio).ok());
    ASSERT_TRUE(
        description.connect({gain.value(), 0}, {output.value(), 0}, graph::MediaType::Audio).ok());

    audio::AudioEngine engine(audio::EngineConfig{});
    ASSERT_TRUE(engine.setGraph(description).ok());
    ASSERT_TRUE(engine.commit().ok());

    std::array<float, 512> in{};
    std::array<float, 512> out{};
    in.fill(0.25F);
    float* inputs[] = {in.data(), in.data()};
    float* outputs[] = {out.data(), out.data()};

    rt::resetViolations();
    std::atomic<int> allocations{0};
    rt::setAllocationHook(countAllocation, &allocations);
    for (int block = 0; block < 16; ++block)
    {
        engine.process(inputs, outputs, 512, static_cast<std::uint64_t>(block * 512));
    }
    rt::setAllocationHook(nullptr, nullptr);

    EXPECT_EQ(rt::violationCount(), 0U);
    EXPECT_EQ(allocations.load(), 0);
    EXPECT_FLOAT_EQ(out[0], 0.25F);
}
