#include <gtest/gtest.h>

#include "axf/core/rt.hpp"
#include "axf/devices/recording_callback.hpp"
#include "axf/media/recorder.hpp"
#include "axf/media/wav_file.hpp"

#include <array>
#include <atomic>
#include <filesystem>
#include <string>
#include <vector>

using namespace axf;

namespace
{
void countAllocation(std::size_t, bool, void* userData) noexcept
{
    static_cast<std::atomic<int>*>(userData)->fetch_add(1, std::memory_order_relaxed);
}

class MarkerCallback final : public devices::IAudioDeviceCallback
{
  public:
    void onAudioCallback(float* const*, float* const* outputs, std::uint32_t frames,
                         std::uint64_t position) noexcept override
    {
        ++calls;
        if (outputs != nullptr && outputs[0] != nullptr && frames > 0)
        {
            outputs[0][0] = static_cast<float>(position);
        }
    }
    std::atomic<int> calls{0};
};
} // namespace

TEST(AudioRecorder, CapturesBlocksAndWritesReadableWav)
{
    media::AudioRecorder recorder;
    ASSERT_TRUE(recorder.start(48000, 2, 64).ok());
    EXPECT_TRUE(recorder.isRecording());

    std::array<float, 32> left{};
    std::array<float, 32> right{};
    left.fill(0.5F);
    right.fill(-0.25F);
    float* inputs[] = {left.data(), right.data()};
    recorder.capture(inputs, 32);
    recorder.capture(inputs, 32);
    EXPECT_EQ(recorder.capturedFrames(), 64U);

    const auto path = (std::filesystem::temp_directory_path() / "axf-recorder-test.wav").string();
    ASSERT_TRUE(recorder.stopAndWrite(path).ok());
    EXPECT_FALSE(recorder.isRecording());

    media::WavReader reader;
    ASSERT_TRUE(reader.open(path).ok());
    const auto info = reader.info();
    ASSERT_TRUE(info.ok());
    EXPECT_EQ(info.value().sampleRate, 48000U);
    EXPECT_EQ(info.value().channels, 2U);
    EXPECT_EQ(info.value().frames, 64U);

    std::vector<float> audio(64 * 2);
    const auto read = reader.read(audio.data(), 64);
    ASSERT_TRUE(read.ok());
    EXPECT_EQ(read.value(), 64U);
    EXPECT_NEAR(audio[0], 0.5F, 1.0F / 32768.0F);
    EXPECT_NEAR(audio[1], -0.25F, 1.0F / 32768.0F);
    std::filesystem::remove(path);
}

TEST(AudioRecorder, DropsFramesBeyondCapacity)
{
    media::AudioRecorder recorder;
    ASSERT_TRUE(recorder.start(48000, 1, 4).ok());
    std::array<float, 6> input{};
    input.fill(0.5F);
    float* inputs[] = {input.data()};
    recorder.capture(inputs, 6);
    EXPECT_EQ(recorder.capturedFrames(), 4U);
    EXPECT_EQ(recorder.droppedFrames(), 2U);
    recorder.stop();
    EXPECT_FALSE(recorder.isRecording());
}

TEST(AudioRecorder, CaptureDoesNotAllocateOnAudioThread)
{
    core::rt::resetViolations();
    std::atomic<int> allocations{0};
    core::rt::setAllocationHook(countAllocation, &allocations);
    media::AudioRecorder recorder;
    ASSERT_TRUE(recorder.start(48000, 2, 8).ok());

    std::array<float, 8> channel{};
    float* inputs[] = {channel.data(), channel.data()};
    {
        core::rt::ScopedAudioThread guard;
        recorder.capture(inputs, 8);
    }
    core::rt::setAllocationHook(nullptr, nullptr);
    EXPECT_EQ(core::rt::violationCount(), 0U);
    EXPECT_EQ(allocations.load(), 0);
    EXPECT_EQ(recorder.capturedFrames(), 8U);
}

TEST(AudioRecorder, DeviceCallbackForwardsAndRecords)
{
    media::AudioRecorder recorder;
    ASSERT_TRUE(recorder.start(48000, 1, 8).ok());
    MarkerCallback marker;
    devices::RecordingDeviceCallback callback(marker, recorder);
    std::array<float, 8> input{};
    std::array<float, 8> output{};
    float* inputs[] = {input.data()};
    float* outputs[] = {output.data()};
    callback.onAudioCallback(inputs, outputs, 8, 123);
    EXPECT_EQ(marker.calls.load(), 1);
    EXPECT_FLOAT_EQ(output[0], 123.0F);
    EXPECT_EQ(recorder.capturedFrames(), 8U);
}
