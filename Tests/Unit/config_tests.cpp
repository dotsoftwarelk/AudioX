#include <gtest/gtest.h>

#include "axf/core/config.hpp"

#include <cstdio>
#include <string>

using namespace axf::core;

TEST(ConfigStore, SetPublishesSnapshotForLockFreeReads)
{
    ConfigStore store;
    ASSERT_TRUE(store.set("gain", "0.5").ok());
    ASSERT_TRUE(store.set("channels", "2").ok());
    ASSERT_TRUE(store.set("mute", "true").ok());

    store.withSnapshot(
        [](const ConfigSnapshot& snapshot)
        {
            EXPECT_TRUE(snapshot.contains("gain"));
            EXPECT_FLOAT_EQ(snapshot.getFloat("gain", 1.0F), 0.5F);
            EXPECT_EQ(snapshot.getInt("channels", 0), 2);
            EXPECT_TRUE(snapshot.getBool("mute", false));
            EXPECT_EQ(snapshot.getString("missing", "fallback"), "fallback");
            EXPECT_EQ(snapshot.size(), 3U);
        });

    const auto value = store.get("gain");
    ASSERT_TRUE(value.ok());
    EXPECT_EQ(value.value(), "0.5");
    EXPECT_EQ(store.get("missing").error().code, ErrorCode::NotFound);
}

TEST(ConfigStore, RejectsInvalidKeysAndRemovesValues)
{
    ConfigStore store;
    EXPECT_EQ(store.set("", "x").error().code, ErrorCode::InvalidArgument);
    EXPECT_EQ(store.set("bad=key", "x").error().code, ErrorCode::InvalidArgument);
    ASSERT_TRUE(store.set("key", "value").ok());
    EXPECT_TRUE(store.remove("key").value());
    EXPECT_FALSE(store.remove("key").value());
}

TEST(ConfigStore, FileRoundTrip)
{
    const std::string path = "axf_config_roundtrip_test.ini";
    ConfigStore store;
    ASSERT_TRUE(store.set("rate", "48000").ok());
    ASSERT_TRUE(store.set("name", "mixdown").ok());
    ASSERT_TRUE(store.saveToFile(path).ok());

    ConfigStore restored;
    ASSERT_TRUE(restored.loadFromFile(path).ok());
    restored.withSnapshot(
        [](const ConfigSnapshot& snapshot)
        {
            EXPECT_EQ(snapshot.getInt("rate", 0), 48000);
            EXPECT_EQ(snapshot.getString("name", ""), "mixdown");
        });
    std::remove(path.c_str());
}
