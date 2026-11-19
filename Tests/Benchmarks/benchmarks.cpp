#include "axf/audio/buffer.hpp"
#include "axf/core/spsc_ring_buffer.hpp"
#include "axf/dsp/utility.hpp"

#include <chrono>
#include <cstdint>
#include <cstdio>

int main()
{
    using clock = std::chrono::steady_clock;

    constexpr std::size_t kQueueOps = 200000;
    axf::core::SpscRingBuffer<std::uint64_t, 1024> queue;
    const auto queueStart = clock::now();
    std::uint64_t produced = 0;
    std::uint64_t consumed = 0;
    for (std::size_t i = 0; i < kQueueOps; ++i)
    {
        while (!queue.tryPush(static_cast<std::uint64_t>(i)))
        {
            std::uint64_t value = 0;
            (void)queue.tryPop(value);
            ++consumed;
        }
        ++produced;
    }
    const auto queueEnd = clock::now();
    const double queueSeconds = std::chrono::duration<double>(queueEnd - queueStart).count();
    const double queueOpsPerSecond =
        queueSeconds > 0.0 ? static_cast<double>(produced + consumed) / queueSeconds : 0.0;

    constexpr std::size_t kFrames = 512;
    constexpr int kIterations = 2000;
    axf::audio::AudioBuffer buffer(2, kFrames);
    buffer.fill(0.5F);
    auto view = axf::audio::makeView(buffer);
    volatile float checksum = 0.0F;
    const auto dspStart = clock::now();
    for (int i = 0; i < kIterations; ++i)
    {
        axf::dsp::applyGain(view, 0.99F);
        checksum += buffer.channelData(0)[kFrames - 1];
    }
    const auto dspEnd = clock::now();
    const double dspSeconds = std::chrono::duration<double>(dspEnd - dspStart).count();
    const double framesPerSecond =
        dspSeconds > 0.0 ? static_cast<double>(kFrames * kIterations) / dspSeconds : 0.0;

    std::printf("{\"spsc_ops_per_sec\": %.0f, \"gain_frames_per_sec\": %.0f, \"checksum\": %.3f}\n",
                queueOpsPerSecond, framesPerSecond, static_cast<double>(checksum));
    return queueOpsPerSecond > 0.0 && framesPerSecond > 0.0 ? 0 : 1;
}
