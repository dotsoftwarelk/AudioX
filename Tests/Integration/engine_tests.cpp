#include <gtest/gtest.h>

#include "axf/audio/buffer_pool.hpp"
#include "axf/audio/engine.hpp"
#include "axf/audio/scheduler.hpp"
#include "axf/core/event.hpp"
#include "axf/dsp/gain_processor.hpp"

#include <array>
#include <cmath>
#include <memory>

using namespace axf;

namespace
{
std::array<float, 128> makeInput(float value)
{
    std::array<float, 128> result{};
    result.fill(value);
    return result;
}
class LatencyProcessor final : public audio::INodeProcessor
{
  public:
    explicit LatencyProcessor(std::uint32_t latency) noexcept : latency_(latency)
    {
    }
    void prepare(double, std::uint32_t) noexcept override
    {
    }
    void reset() noexcept override
    {
    }
    void process(audio::ProcessContext&) noexcept override
    {
    }
    [[nodiscard]] std::uint32_t latencySamples() const noexcept override
    {
        return latency_;
    }

  private:
    std::uint32_t latency_;
};

struct Payload final
{
    static constexpr core::events::EventTypeId typeId = 7;
    int value;
};
} // namespace

TEST(AudioEngine, CompilesAndProcessesGainChain)
{
    audio::EngineConfig config;
    config.sampleRate = 48000;
    config.maxFrames = 512;
    config.inputChannels = 2;
    config.outputChannels = 2;

    graph::Graph description;
    const auto input = description.addNode(graph::NodeKind::AudioInput, "in", 0, 1);
    const auto gain = description.addNode(graph::NodeKind::Gain, "gain", 1, 1);
    const auto output = description.addNode(graph::NodeKind::AudioOutput, "out", 1, 0);
    ASSERT_TRUE(input && gain && output);
    ASSERT_TRUE(
        description.connect({input.value(), 0}, {gain.value(), 0}, graph::MediaType::Audio).ok());
    ASSERT_TRUE(
        description.connect({gain.value(), 0}, {output.value(), 0}, graph::MediaType::Audio).ok());

    audio::AudioEngine engine(config);
    engine.setProcessor(gain.value(), std::make_unique<dsp::GainProcessor>(0.5F));
    ASSERT_TRUE(engine.setGraph(description).ok());
    ASSERT_TRUE(engine.commit().ok());

    const audio::CompiledGraph* compiled = engine.activeGraph();
    ASSERT_NE(compiled, nullptr);
    EXPECT_TRUE(compiled->diagnostics().valid);
    EXPECT_EQ(compiled->nodes().size(), 3U);
    EXPECT_EQ(compiled->edges().size(), 2U);
    EXPECT_EQ(engine.latencySamples(), 0U);

    auto left = makeInput(1.0F);
    auto right = makeInput(-0.5F);
    std::array<float, 128> outLeft{};
    std::array<float, 128> outRight{};
    float* inputs[] = {left.data(), right.data()};
    float* outputs[] = {outLeft.data(), outRight.data()};
    engine.process(inputs, outputs, 128, 0);

    EXPECT_FLOAT_EQ(outLeft[0], 0.5F);
    EXPECT_FLOAT_EQ(outRight[0], -0.25F);
    EXPECT_FLOAT_EQ(outLeft[127], 0.5F);
}

TEST(AudioEngine, RejectsChannelMismatch)
{
    graph::Graph description;
    const auto input = description.addNode(graph::NodeKind::AudioInput, "in", 0, 1, 0, 0, 1);
    const auto output = description.addNode(graph::NodeKind::AudioOutput, "out", 1, 0, 0, 0, 1);
    ASSERT_TRUE(input && output);
    ASSERT_TRUE(
        description.connect({input.value(), 0}, {output.value(), 0}, graph::MediaType::Audio).ok());

    audio::AudioEngine engine(audio::EngineConfig{});
    ASSERT_TRUE(engine.setGraph(description).ok());
    const auto result = engine.commit();
    EXPECT_FALSE(result.ok());
    EXPECT_EQ(result.error().code, core::ErrorCode::Unsupported);
    EXPECT_EQ(engine.activeGraph(), nullptr);
}

TEST(AudioEngine, PublishesNewGraphWithoutLosingOldOne)
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
    engine.setProcessor(gain.value(), std::make_unique<dsp::GainProcessor>(0.5F));
    ASSERT_TRUE(engine.setGraph(description).ok());
    ASSERT_TRUE(engine.commit().ok());

    auto in = makeInput(1.0F);
    std::array<float, 128> out{};
    float* inputs[] = {in.data(), in.data()};
    float* outputs[] = {out.data(), out.data()};
    engine.process(inputs, outputs, 128, 0);
    EXPECT_FLOAT_EQ(out[0], 0.5F);

    engine.setProcessor(gain.value(), std::make_unique<dsp::GainProcessor>(0.25F));
    ASSERT_TRUE(engine.setGraph(description).ok());
    ASSERT_TRUE(engine.commit().ok());
    out.fill(0.0F);
    engine.process(inputs, outputs, 128, 128);
    EXPECT_FLOAT_EQ(out[0], 0.25F);
}

TEST(AudioEngine, LongRunStaysFinite)
{
    graph::Graph description;
    const auto input = description.addNode(graph::NodeKind::AudioInput, "in", 0, 1);
    const auto output = description.addNode(graph::NodeKind::AudioOutput, "out", 1, 0);
    ASSERT_TRUE(input && output);
    ASSERT_TRUE(
        description.connect({input.value(), 0}, {output.value(), 0}, graph::MediaType::Audio).ok());

    audio::AudioEngine engine(audio::EngineConfig{});
    ASSERT_TRUE(engine.setGraph(description).ok());
    ASSERT_TRUE(engine.commit().ok());

    std::array<float, 64> in{};
    std::array<float, 64> out{};
    in.fill(0.125F);
    float* inputs[] = {in.data(), in.data()};
    float* outputs[] = {out.data(), out.data()};
    for (int block = 0; block < 2000; ++block)
    {
        engine.process(inputs, outputs, 64, static_cast<std::uint64_t>(block * 64));
        ASSERT_TRUE(std::isfinite(out[0]));
        EXPECT_FLOAT_EQ(out[0], 0.125F);
    }
}

TEST(AudioBufferPool, PreallocatesAllBuffers)
{
    audio::AudioBufferPool pool(2, 64, 4);
    EXPECT_EQ(pool.size(), 4U);
    EXPECT_NE(pool.tryGet(0), nullptr);
    EXPECT_EQ(pool.tryGet(4), nullptr);
    pool.get(0).fill(0.5F);
    EXPECT_FLOAT_EQ(pool.get(0).channelData(1)[63], 0.5F);
}

TEST(AudioScheduler, SampleAccurateOrderingAndPayload)
{
    audio::EventScheduler scheduler(8);
    Payload first{11};
    Payload second{22};
    Payload third{33};

    ASSERT_TRUE(scheduler.trySchedule(9, Payload::typeId, &third, sizeof(third)));
    ASSERT_TRUE(scheduler.trySchedule(5, Payload::typeId, &first, sizeof(first)));
    ASSERT_TRUE(scheduler.trySchedule(5, Payload::typeId, &second, sizeof(second)));

    audio::ScheduledEvent event{};
    ASSERT_TRUE(scheduler.popDue(5, event));
    EXPECT_EQ(event.frame, 5U);
    EXPECT_EQ(event.sequence, 1U);
    EXPECT_EQ(core::events::asEvent<Payload>(event.payload)->value, 11);

    ASSERT_TRUE(scheduler.popDue(5, event));
    EXPECT_EQ(event.sequence, 2U);
    EXPECT_EQ(core::events::asEvent<Payload>(event.payload)->value, 22);

    EXPECT_FALSE(scheduler.popDue(8, event));
    ASSERT_TRUE(scheduler.popDue(9, event));
    EXPECT_EQ(core::events::asEvent<Payload>(event.payload)->value, 33);
    EXPECT_EQ(scheduler.count(), 0U);
}

TEST(AudioEngine, PropagatesLatencyToOutput)
{
    graph::Graph description;
    const auto input = description.addNode(graph::NodeKind::AudioInput, "in", 0, 1);
    const auto gain = description.addNode(graph::NodeKind::Gain, "delay", 1, 1);
    const auto output = description.addNode(graph::NodeKind::AudioOutput, "out", 1, 0);
    ASSERT_TRUE(input && gain && output);
    ASSERT_TRUE(
        description.connect({input.value(), 0}, {gain.value(), 0}, graph::MediaType::Audio).ok());
    ASSERT_TRUE(
        description.connect({gain.value(), 0}, {output.value(), 0}, graph::MediaType::Audio).ok());

    audio::AudioEngine engine(audio::EngineConfig{});
    engine.setProcessor(gain.value(), std::make_unique<LatencyProcessor>(16));
    ASSERT_TRUE(engine.setGraph(description).ok());
    ASSERT_TRUE(engine.commit().ok());
    EXPECT_EQ(engine.latencySamples(), 16U);
    EXPECT_EQ(engine.diagnostics().latencySamples, 16U);
}
