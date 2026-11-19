#include <gtest/gtest.h>

#include "axf/core/rt.hpp"
#include "axf/dsp/biquad.hpp"
#include "axf/dsp/panner.hpp"

#include <array>
#include <atomic>

using namespace axf;

namespace
{
void countAllocation(std::size_t, bool, void* userData) noexcept
{
    static_cast<std::atomic<int>*>(userData)->fetch_add(1, std::memory_order_relaxed);
}
} // namespace

TEST(DspAudit, Phase4ProcessorsDoNotAllocate)
{
    dsp::BiquadDesign design;
    design.type = dsp::BiquadType::Peaking;
    design.frequencyHz = 1200.0;
    design.gainDb = 3.0;
    const auto coefficients = dsp::designBiquad(design, 48000.0);
    ASSERT_TRUE(coefficients.ok());
    dsp::BiquadProcessor biquad(coefficients.value());
    dsp::PannerProcessor panner;

    std::array<float, 64> mono{};
    mono.fill(0.25F);
    std::array<float, 64> left{};
    std::array<float, 64> right{};
    float* sourcePointers[]{mono.data()};
    float* leftPointers[]{left.data()};
    float* rightPointers[]{right.data()};
    audio::AudioBufferView sourceView{sourcePointers, 1, mono.size()};
    audio::AudioBufferView outLeft{leftPointers, 1, left.size()};
    audio::AudioBufferView outRight{rightPointers, 1, right.size()};
    audio::AudioBufferView outputs[2]{outLeft, outRight};
    audio::ProcessContext context{&sourceView, 1, outputs, 2, 64, 48000.0, 0};

    core::rt::resetViolations();
    std::atomic<int> allocations{0};
    core::rt::setAllocationHook(countAllocation, &allocations);
    {
        core::rt::ScopedAudioThread guard;
        for (int block = 0; block < 32; ++block)
        {
            biquad.process(context);
            panner.process(context);
        }
    }
    core::rt::setAllocationHook(nullptr, nullptr);

    EXPECT_EQ(core::rt::violationCount(), 0U);
    EXPECT_EQ(allocations.load(), 0);
    EXPECT_TRUE(std::isfinite(left[0]));
}
