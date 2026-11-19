#include <gtest/gtest.h>

#include "axf/plugins/plugin_host.hpp"

#include <array>
#include <vector>

using namespace axf;

namespace
{
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
