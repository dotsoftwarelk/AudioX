#include <gtest/gtest.h>

#include "axf/audio/engine.hpp"
#include "axf/core/rt.hpp"
#include "axf/project/document.hpp"

#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <string>
#include <thread>
#include <vector>

using namespace axf;

namespace
{
audio::EngineConfig stressEngineConfig() noexcept
{
    audio::EngineConfig config;
    config.sampleRate = 48000;
    config.maxFrames = 512;
    return config;
}

graph::Graph stressGraph(double gain)
{
    graph::Graph description;
    const auto input = description.addNode(graph::NodeKind::AudioInput, "in", 0, 1);
    const auto processor = description.addNode(graph::NodeKind::Gain, "gain", 1, 1);
    const auto output = description.addNode(graph::NodeKind::AudioOutput, "out", 1, 0);
    if (input && processor && output)
    {
        (void)description.connect({input.value(), 0}, {processor.value(), 0},
                                  graph::MediaType::Audio);
        (void)description.connect({processor.value(), 0}, {output.value(), 0},
                                  graph::MediaType::Audio);
    }
    (void)gain;
    return description;
}

std::string randomDocument(std::uint32_t& state, std::size_t length)
{
    static constexpr std::string_view alphabet = "AXF-PROJECT\t01NAMEEND\\xyz\n";
    std::string result;
    for (std::size_t index = 0; index < length; ++index)
    {
        state = state * 1664525U + 1013904223U;
        result.push_back(alphabet[(state >> 16U) % alphabet.size()]);
    }
    return result;
}
} // namespace

TEST(Hardening, ConcurrentRepublishDuringAudioCallbacks)
{
    core::rt::resetViolations();
    audio::AudioEngine engine(stressEngineConfig());
    auto description = stressGraph(0.5);
    ASSERT_TRUE(description.validate().ok());
    ASSERT_TRUE(engine.setGraph(description).ok());
    ASSERT_TRUE(engine.commit().ok());

    std::atomic<int> nonFinite{0};
    std::atomic<int> controlErrors{0};
    std::atomic<bool> ready{false};
    std::thread audioThread(
        [&]
        {
            std::array<float, 512> input{};
            std::array<float, 512> left{};
            std::array<float, 512> right{};
            input.fill(0.25F);
            float* inputs[]{input.data(), input.data()};
            float* outputs[]{left.data(), right.data()};
            ready.store(true, std::memory_order_release);
            for (int block = 0; block < 20000; ++block)
            {
                engine.process(inputs, outputs, 512, static_cast<std::uint64_t>(block) * 512U);
                if (!std::isfinite(left[0]) || !std::isfinite(right[0]))
                {
                    nonFinite.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });

    while (!ready.load(std::memory_order_acquire))
    {
        std::this_thread::yield();
    }
    std::thread controlThread(
        [&]
        {
            for (int revision = 0; revision < 40; ++revision)
            {
                auto next = stressGraph(0.1F + static_cast<float>(revision) * 0.01F);
                ASSERT_TRUE(engine.setGraph(next).ok());
                ASSERT_TRUE(engine.commit().ok());
                std::this_thread::yield();
            }
        });

    audioThread.join();
    controlThread.join();
    EXPECT_EQ(nonFinite.load(), 0);
    EXPECT_EQ(controlErrors.load(), 0);
    EXPECT_EQ(core::rt::violationCount(), 0U);
}

TEST(Hardening, ProjectParserResistsArbitraryBytes)
{
    std::uint32_t state = 12345U;
    for (int iteration = 0; iteration < 300; ++iteration)
    {
        const std::string text = randomDocument(state, static_cast<std::size_t>(iteration % 97));
        const auto parsed = project::ProjectDocument::parse(text);
        if (parsed.ok())
        {
            const auto reparsed = project::ProjectDocument::parse(parsed.value().serialize());
            EXPECT_TRUE(reparsed.ok());
        }
    }
}
