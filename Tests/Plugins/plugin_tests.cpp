#include <gtest/gtest.h>

#include "axf/plugins/plugin_host.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

using namespace axf;

namespace
{
[[maybe_unused]] const char* environmentValue(const char* name) noexcept
{
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4996)
#endif
    return std::getenv(name);
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
}

class PassThroughPlugin final : public plugins::IPluginInstance
{
  public:
    void prepare(double, std::uint32_t) noexcept override
    {
        prepared_ = true;
    }
    void reset() noexcept override
    {
        prepared_ = false;
    }
    void process(audio::AudioBufferView& audio, midi::MidiEventBufferView& midi) noexcept override
    {
        if (!prepared_ || !audio.valid())
        {
            return;
        }
        dspOutput_ = audio.channel(0)[0];
        midiProcessed_ = midi.size;
    }
    std::uint32_t latencySamples() const noexcept override
    {
        return 0;
    }
    core::Result<std::vector<std::byte>> getState() const override
    {
        return state_;
    }
    core::Result<void> setState(const std::vector<std::byte>& state) override
    {
        state_ = state;
        return core::success();
    }
    float dspOutput() const noexcept
    {
        return dspOutput_;
    }
    std::size_t midiProcessed() const noexcept
    {
        return midiProcessed_;
    }

  private:
    bool prepared_{false};
    float dspOutput_{0.0F};
    std::size_t midiProcessed_{0};
    std::vector<std::byte> state_{std::byte{1}, std::byte{2}};
};
} // namespace

TEST(PluginHost, InterfaceSupportsProcessingAndState)
{
    PassThroughPlugin plugin;
    plugin.prepare(48000.0, 256);
    audio::AudioBuffer buffer(1, 8);
    buffer.channelData(0)[0] = 0.42F;
    auto view = audio::makeView(buffer);
    std::array<midi::MidiEvent, 4> events{};
    midi::MidiEventBufferView midi{events.data(), 0, events.size()};
    ASSERT_TRUE(midi.push(midi::makeNoteOn(0, 0, 60, 100)));
    plugin.process(view, midi);
    EXPECT_FLOAT_EQ(plugin.dspOutput(), 0.42F);
    EXPECT_EQ(plugin.midiProcessed(), 1U);

    const auto state = plugin.getState();
    ASSERT_TRUE(state.ok());
    EXPECT_EQ(state.value().size(), 2U);
    EXPECT_TRUE(plugin.setState(std::vector<std::byte>{std::byte{9}}).ok());
    EXPECT_EQ(plugin.getState().value().size(), 1U);
    plugin.reset();
    EXPECT_EQ(plugin.latencySamples(), 0U);
}

#if defined(AXF_HAS_VST3) && AXF_HAS_VST3
TEST(PluginHost, Vst3HostScansEmptyDirectory)
{
    auto host = plugins::createVst3Host();
    ASSERT_NE(host, nullptr);
    const auto directory = std::filesystem::temp_directory_path() / "axf-vst3-empty-scan";
    std::filesystem::create_directories(directory);
    const auto descriptors = host->scan(directory.string());
    ASSERT_TRUE(descriptors.ok());
    EXPECT_TRUE(descriptors.value().empty());

    plugins::PluginDescriptor missing;
    missing.id = "not-a-uid";
    missing.path = (directory / "missing.vst3").string();
    EXPECT_FALSE(host->create(missing).ok());
}
#else
TEST(PluginHost, Vst3HostUnavailableWithoutSdk)
{
    EXPECT_EQ(plugins::createVst3Host(), nullptr);
}
#endif

#if defined(AXF_HAS_VST3) && AXF_HAS_VST3
// Opt-in smoke test against a real VST3 plug-in binary. Set
// AXF_VST3_PLUGIN_PATH to a .vst3 file or a directory of plug-ins; when the
// plug-in is a unity-gain effect, also set AXF_VST3_EXPECT_GAIN=1. This runs
// scanning, loading, preparation, audio processing and state round-trip.
TEST(PluginHost, Vst3RealPluginSmoke)
{
    const char* configured = environmentValue("AXF_VST3_PLUGIN_PATH");
    if (configured == nullptr || *configured == '\0')
    {
        GTEST_SKIP() << "set AXF_VST3_PLUGIN_PATH to run the real plug-in smoke test";
    }
    const std::filesystem::path requested(configured);
    const std::filesystem::path directory =
        requested.has_extension() ? requested.parent_path() : requested;
    const bool wantGain = environmentValue("AXF_VST3_EXPECT_GAIN") != nullptr;

    auto host = plugins::createVst3Host();
    ASSERT_NE(host, nullptr);
    const auto descriptors = host->scan(directory.string());
    ASSERT_TRUE(descriptors.ok());
    ASSERT_FALSE(descriptors.value().empty());

    auto selected = descriptors.value().end();
    for (auto it = descriptors.value().begin(); it != descriptors.value().end(); ++it)
    {
        if (requested.has_extension() && it->path != requested.string())
        {
            continue;
        }
        if (wantGain && it->name.find("AGain") == std::string::npos)
        {
            continue;
        }
        selected = it;
        break;
    }
    if (selected == descriptors.value().end())
    {
        for (const auto& descriptor : descriptors.value())
        {
            std::cerr << "scanned: " << descriptor.name << " | " << descriptor.path << '\n';
        }
    }
    ASSERT_NE(selected, descriptors.value().end())
        << "no matching VST3 class found at " << requested.string();

    auto created = host->create(*selected);
    ASSERT_TRUE(created.ok());
    auto instance = std::move(created.value());
    instance->prepare(48000.0, 128);

    audio::AudioBuffer audio(2, 128);
    audio.fill(0.25F);
    auto view = audio::makeView(audio);
    std::array<midi::MidiEvent, 8> events{};
    midi::MidiEventBufferView midi{events.data(), 0, events.size()};
    instance->process(view, midi);

    if (wantGain)
    {
        EXPECT_NEAR(audio.channelData(0)[0], 0.25F, 1.0e-5F);
        EXPECT_NEAR(audio.channelData(1)[127], 0.25F, 1.0e-5F);
    }
    for (core::ChannelCount channel = 0; channel < audio.channels(); ++channel)
    {
        for (std::size_t frame = 0; frame < audio.frames(); ++frame)
        {
            EXPECT_TRUE(std::isfinite(audio.channelData(channel)[frame]));
        }
    }

    const auto state = instance->getState();
    if (!state.ok())
    {
        std::cerr << "getState failed: " << state.error().message << '\n';
    }
    ASSERT_TRUE(state.ok());
    EXPECT_TRUE(instance->setState(state.value()).ok());
    instance->reset();
}
#endif
