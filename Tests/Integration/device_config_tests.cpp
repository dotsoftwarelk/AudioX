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
}
