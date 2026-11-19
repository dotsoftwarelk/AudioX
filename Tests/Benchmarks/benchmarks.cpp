#include "axf/audio/buffer.hpp"
#include "axf/core/spsc_ring_buffer.hpp"
#include "axf/dsp/biquad.hpp"
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

    const auto design = axf::dsp::designBiquad({}, 48000.0);
    if (!design.ok())
    {
        return 1;
    }
    axf::dsp::BiquadProcessor biquad(design.value());
    axf::audio::AudioBuffer filtered(1, kFrames);
    filtered.fill(0.5F);
    auto filteredView = axf::audio::makeView(filtered);
    volatile float biquadChecksum = 0.0F;
    const auto biquadStart = clock::now();
    for (int i = 0; i < kIterations; ++i)
    {
        biquad.process(filteredView);
        biquadChecksum += filtered.channelData(0)[kFrames - 1];
    }
    const auto biquadEnd = clock::now();
    const double biquadSeconds = std::chrono::duration<double>(biquadEnd - biquadStart).count();
    const double biquadFramesPerSecond =
        biquadSeconds > 0.0 ? static_cast<double>(kFrames * kIterations) / biquadSeconds : 0.0;

    std::printf("{\"spsc_ops_per_sec\": %.0f, \"gain_frames_per_sec\": %.0f, "
                "\"biquad_frames_per_sec\": %.0f, \"checksum\": %.3f, \"biquad_checksum\": %.3f}\n",
                queueOpsPerSecond, framesPerSecond, biquadFramesPerSecond,
                static_cast<double>(checksum), static_cast<double>(biquadChecksum));
    return queueOpsPerSecond > 0.0 && framesPerSecond > 0.0 && biquadFramesPerSecond > 0.0 ? 0 : 1;
}
