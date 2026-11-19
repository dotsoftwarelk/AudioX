#include <gtest/gtest.h>

#include "axf/devices/device_manager.hpp"

#include <atomic>
#include <memory>
#include <string>
#include <vector>

using namespace axf;

namespace
{
class FakeDevice final : public devices::IAudioDevice
{
  public:
    FakeDevice(devices::DeviceInfo info, devices::DeviceConfig config)
        : info_(std::move(info)), config_(config)
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
    [[nodiscard]] core::Result<void> start(devices::IAudioDeviceCallback*) override
    {
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
        return 0.01;
    }
    [[nodiscard]] devices::DeviceState state() const noexcept override
    {
        return devices::DeviceState::Active;
    }
    [[nodiscard]] devices::DeviceDiagnostics diagnostics() const noexcept override
    {
        return devices::DeviceDiagnostics{};
    }

  private:
    devices::DeviceInfo info_;
    devices::DeviceConfig config_;
    bool running_{false};
};

class FakeBackend final : public devices::IDeviceBackend
{
  public:
    explicit FakeBackend(devices::Backend backend) : backend_(backend)
    {
    }

    [[nodiscard]] devices::Backend backend() const noexcept override
    {
        return backend_;
    }

    [[nodiscard]] core::Result<std::vector<devices::DeviceInfo>> enumerate() override
    {
        devices::DeviceInfo info;
        info.id = backend_ == devices::Backend::WasapiShared ? "shared-1" : "exclusive-1";
        info.name = backend_ == devices::Backend::WasapiShared ? "Speakers" : "Headphones";
        info.backend = backend_;
        info.maxInputChannels = 0;
        info.maxOutputChannels = 2;
        info.supportedSampleRates = {48000, 96000};
        return std::vector<devices::DeviceInfo>{info};
    }

    [[nodiscard]] core::Result<std::unique_ptr<devices::IAudioDevice>> open(
        const std::string& deviceId, const devices::DeviceConfig& config) override
    {
        lastId = deviceId;
        devices::DeviceInfo info;
        info.id = deviceId;
        info.backend = backend_;
        info.maxOutputChannels = 2;
        std::unique_ptr<devices::IAudioDevice> device =
            std::make_unique<FakeDevice>(std::move(info), config);
        return device;
    }

    std::string lastId;

  private:
    devices::Backend backend_;
};

class CountingListener final : public devices::IDeviceChangeListener
{
  public:
    void onDevicesChanged() noexcept override
    {
        count.fetch_add(1, std::memory_order_relaxed);
    }
    std::atomic<int> count{0};
};
} // namespace

TEST(DeviceManager, AggregatesAndSortsDevices)
{
    devices::DeviceManager manager;
    manager.addBackend(std::make_unique<FakeBackend>(devices::Backend::WasapiExclusive));
    manager.addBackend(std::make_unique<FakeBackend>(devices::Backend::WasapiShared));
    EXPECT_EQ(manager.backendCount(), 2U);

    const auto devices = manager.enumerate();
    ASSERT_TRUE(devices.ok());
    ASSERT_EQ(devices.value().size(), 2U);
    EXPECT_EQ(devices.value()[0].backend, devices::Backend::WasapiShared);
    EXPECT_EQ(devices.value()[1].backend, devices::Backend::WasapiExclusive);

    const auto output = manager.defaultDevice(devices::Backend::WasapiShared, false);
    ASSERT_TRUE(output.ok());
    EXPECT_EQ(output.value().id, "shared-1");
    EXPECT_FALSE(manager.defaultDevice(devices::Backend::WasapiShared, true).ok());
}

TEST(DeviceManager, OpensWithBackendFilter)
{
    devices::DeviceManager manager;
    auto* backend = new FakeBackend(devices::Backend::WasapiExclusive);
    manager.addBackend(std::unique_ptr<devices::IDeviceBackend>(backend));

    devices::DeviceConfig config;
    config.backend = devices::Backend::WasapiExclusive;
    auto device = manager.open("device-42", config);
    ASSERT_TRUE(device.ok());
    EXPECT_EQ(backend->lastId, "device-42");
    EXPECT_EQ(device.value()->state(), devices::DeviceState::Active);
    EXPECT_TRUE(device.value()->start(nullptr).ok());
    EXPECT_TRUE(device.value()->isRunning());
    device.value()->stop();
    EXPECT_FALSE(device.value()->isRunning());
}

TEST(DeviceManager, NotifiesListeners)
{
    devices::DeviceManager manager;
    CountingListener listener;
    manager.addChangeListener(&listener);
    manager.addChangeListener(&listener);
    EXPECT_EQ(manager.listenerCount(), 1U);
    manager.notifyDevicesChanged();
    manager.notifyDevicesChanged();
    EXPECT_EQ(listener.count.load(), 2);
    manager.removeChangeListener(&listener);
    EXPECT_EQ(manager.listenerCount(), 0U);
}

TEST(DeviceManager, DeviceStateNames)
{
    EXPECT_STREQ(devices::deviceStateName(devices::DeviceState::Disconnected), "disconnected");
    EXPECT_STREQ(devices::deviceStateName(devices::DeviceState::NotPresent), "not present");
}

#if !defined(_WIN32)
TEST(DeviceManager, WasapiBackendUnavailableOffWindows)
{
    EXPECT_EQ(devices::createWasapiBackend(), nullptr);
}
#endif

#if defined(AXF_HAS_ASIO) && AXF_HAS_ASIO
TEST(DeviceManager, AsioBackendAvailableWhenBuiltWithSdk)
{
    auto backend = devices::createAsioBackend();
    ASSERT_NE(backend, nullptr);
    EXPECT_EQ(backend->backend(), devices::Backend::Asio);
}
#else
TEST(DeviceManager, AsioBackendUnavailableWithoutSdk)
{
    EXPECT_EQ(devices::createAsioBackend(), nullptr);
}
#endif
