#include <gtest/gtest.h>

#include "axf/audio/engine.hpp"
#include "axf/devices/device.hpp"
#include "axf/dsp/gain_processor.hpp"

#include <array>
#include <memory>
#include <string>

using namespace axf;

namespace
{
class EngineCallback final : public devices::IAudioDeviceCallback
{
  public:
    explicit EngineCallback(audio::AudioEngine& engine) : engine_(engine)
    {
    }

    void onAudioCallback(float* const* inputs, float* const* outputs, std::uint32_t frames,
                         std::uint64_t frame) noexcept override
    {
        engine_.process(inputs, outputs, frames, frame);
    }

  private:
    audio::AudioEngine& engine_;
};

class ManualDevice final : public devices::IAudioDevice
{
  public:
    ManualDevice(devices::DeviceConfig config, devices::IAudioDeviceCallback* callback)
        : info_{"manual", "Manual device", devices::Backend::WasapiShared, 2, 2, {48000}},
          config_(config), callback_(callback)
    {
    }

    [[nodiscard]] const devices::DeviceInfo& info() const noexcept override
    {
        return info_;
    }
    [[nodiscard]] const devices::DeviceConfig& config() const noexcept override
    {
        return config_;
    }
    [[nodiscard]] core::Result<void> start(devices::IAudioDeviceCallback* callback) override
    {
        callback_ = callback;
        running_ = true;
        return core::success();
    }
    void stop() noexcept override
    {
        running_ = false;
    }
    [[nodiscard]] bool isRunning() const noexcept override
    {
        return running_;
    }
    [[nodiscard]] double reportedLatencySeconds() const noexcept override
    {
        return static_cast<double>(config_.bufferFrames) / static_cast<double>(config_.sampleRate);
    }
    [[nodiscard]] devices::DeviceState state() const noexcept override
    {
        return state_;
    }
    [[nodiscard]] devices::DeviceDiagnostics diagnostics() const noexcept override
    {
        return diagnostics_;
    }

    void render(float* const* inputs, float* const* outputs, std::uint32_t frames)
    {
        ++diagnostics_.callbackCount;
        diagnostics_.lastCallbackFrames = frames;
        if (callback_ != nullptr)
        {
            callback_->onAudioCallback(inputs, outputs, frames, streamFrame_);
        }
        streamFrame_ += frames;
    }

    void simulateDisconnect() noexcept
    {
        state_ = devices::DeviceState::Disconnected;
    }

  private:
    devices::DeviceInfo info_;
    devices::DeviceConfig config_;
    devices::IAudioDeviceCallback* callback_{nullptr};
    devices::DeviceState state_{devices::DeviceState::Active};
    devices::DeviceDiagnostics diagnostics_{};
    bool running_{false};
    std::uint64_t streamFrame_{0};
};
} // namespace

TEST(DeviceEngineIntegration, DeviceCallbackDrivesCompiledGraph)
{
    audio::EngineConfig engineConfig;
    engineConfig.sampleRate = 48000;
    engineConfig.maxFrames = 256;
    audio::AudioEngine engine(engineConfig);

    graph::Graph description;
    const auto input = description.addNode(graph::NodeKind::AudioInput, "in", 0, 1);
    const auto gain = description.addNode(graph::NodeKind::Gain, "gain", 1, 1);
    const auto output = description.addNode(graph::NodeKind::AudioOutput, "out", 1, 0);
    ASSERT_TRUE(input && gain && output);
    ASSERT_TRUE(
        description.connect({input.value(), 0}, {gain.value(), 0}, graph::MediaType::Audio).ok());
    ASSERT_TRUE(
        description.connect({gain.value(), 0}, {output.value(), 0}, graph::MediaType::Audio).ok());

    engine.setProcessor(gain.value(), std::make_unique<dsp::GainProcessor>(0.25F));
    ASSERT_TRUE(engine.setGraph(description).ok());
    ASSERT_TRUE(engine.commit().ok());

    devices::DeviceConfig deviceConfig;
    deviceConfig.sampleRate = 48000;
    deviceConfig.bufferFrames = 128;
    deviceConfig.inputChannels = 2;
    deviceConfig.outputChannels = 2;
    deviceConfig.backend = devices::Backend::WasapiShared;
    ASSERT_TRUE(devices::validateConfig(deviceConfig).ok());

    EngineCallback callback(engine);
    ManualDevice device(deviceConfig, &callback);
    ASSERT_TRUE(device.start(&callback).ok());

    std::array<float, 128> left{};
    std::array<float, 128> right{};
    std::array<float, 128> outLeft{};
    std::array<float, 128> outRight{};
    left.fill(0.8F);
    right.fill(-0.4F);
    float* inputs[] = {left.data(), right.data()};
    float* outputs[] = {outLeft.data(), outRight.data()};

    device.render(inputs, outputs, 128);
    EXPECT_FLOAT_EQ(outLeft[0], 0.2F);
    EXPECT_FLOAT_EQ(outRight[0], -0.1F);
    EXPECT_EQ(device.diagnostics().callbackCount, 1U);
    EXPECT_EQ(device.diagnostics().lastCallbackFrames, 128U);
    EXPECT_DOUBLE_EQ(device.reportedLatencySeconds(), 128.0 / 48000.0);

    device.simulateDisconnect();
    EXPECT_EQ(device.state(), devices::DeviceState::Disconnected);
    device.stop();
    EXPECT_FALSE(device.isRunning());
}
