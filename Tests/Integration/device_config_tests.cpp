#include <gtest/gtest.h>

#include "axf/devices/device.hpp"

using namespace axf;

TEST(DeviceConfig, ValidConfigurations)
{
    devices::DeviceConfig config;
    EXPECT_TRUE(devices::validateConfig(config).ok());
    config.backend = devices::Backend::WasapiExclusive;
    config.sampleRate = 96000;
    config.bufferFrames = 128;
    EXPECT_TRUE(devices::validateConfig(config).ok());
    EXPECT_STREQ(devices::backendName(devices::Backend::Asio), "ASIO");
    EXPECT_STREQ(devices::deviceStateName(devices::DeviceState::Active), "active");
    EXPECT_TRUE(devices::validateWasapiConfig(config).ok());
    config.backend = devices::Backend::None;
    EXPECT_EQ(devices::validateWasapiConfig(config).error().code, core::ErrorCode::InvalidArgument);
}

TEST(DeviceConfig, RejectsInvalidSettings)
{
    devices::DeviceConfig config;
    config.sampleRate = 0;
    EXPECT_EQ(devices::validateConfig(config).error().code, core::ErrorCode::OutOfRange);

    config = devices::DeviceConfig{};
    config.bufferFrames = 100;
    EXPECT_EQ(devices::validateConfig(config).error().code, core::ErrorCode::OutOfRange);

    config = devices::DeviceConfig{};
    config.inputChannels = 0;
    config.outputChannels = 0;
    EXPECT_EQ(devices::validateConfig(config).error().code, core::ErrorCode::InvalidArgument);

    config = devices::DeviceConfig{};
    config.backend = devices::Backend::None;
    EXPECT_EQ(devices::validateConfig(config).error().code, core::ErrorCode::InvalidArgument);

    config = devices::DeviceConfig{};
    config.inputChannels = 2;
    EXPECT_EQ(devices::validateWasapiConfig(config).error().code, core::ErrorCode::Unsupported);

    config = devices::DeviceConfig{};
    config.outputChannels = 6;
    config.backend = devices::Backend::WasapiExclusive;
    EXPECT_EQ(devices::validateWasapiConfig(config).error().code, core::ErrorCode::Unsupported);
}
