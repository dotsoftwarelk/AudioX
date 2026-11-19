#pragma once

#include "axf/audio/buffer.hpp"
#include "axf/core/export.hpp"

#include <cstddef>
#include <cstdint>

namespace axf::audio
{

constexpr std::size_t kMaxNodePorts = 8;

struct ProcessContext final
{
    AudioBufferView* inputs{nullptr};
    std::uint16_t inputCount{0};
    AudioBufferView* outputs{nullptr};
    std::uint16_t outputCount{0};
    std::uint32_t frameCount{0};
    double sampleRate{48000.0};
    std::uint64_t streamFrame{0};
};

/// Audio-thread processor contract. Implementations are owned by a compiled
/// graph and must never allocate, lock, perform I/O or wait.
class INodeProcessor
{
  public:
    virtual ~INodeProcessor() = default;
    virtual void prepare(double sampleRate, std::uint32_t maxFrames) noexcept = 0;
    virtual void reset() noexcept = 0;
    virtual void process(ProcessContext& context) noexcept = 0;
    [[nodiscard]] virtual std::uint32_t latencySamples() const noexcept = 0;
};

/// Default processor for nodes without an explicit implementation. Copies the
/// first input port to the first output port, one channel at a time.
class PassThroughProcessor final : public INodeProcessor
{
  public:
    void prepare(double, std::uint32_t) noexcept override
    {
    }
    void reset() noexcept override
    {
    }

    void process(ProcessContext& context) noexcept override
    {
        if (context.inputCount == 0 || context.outputCount == 0 || context.inputs == nullptr ||
            context.outputs == nullptr)
        {
            return;
        }
        const AudioBufferView& source = context.inputs[0];
        AudioBufferView& destination = context.outputs[0];
        const core::ChannelCount channels =
            source.channels < destination.channels ? source.channels : destination.channels;
        const std::size_t frames =
            source.frames < destination.frames ? source.frames : destination.frames;
        for (core::ChannelCount channel = 0; channel < channels; ++channel)
        {
            const float* from = source.channel(channel);
            float* to = destination.channel(channel);
            if (from != nullptr && to != nullptr)
            {
                for (std::size_t frame = 0; frame < frames; ++frame)
                {
                    to[frame] = from[frame];
                }
            }
        }
    }

    [[nodiscard]] std::uint32_t latencySamples() const noexcept override
    {
        return 0;
    }
};

} // namespace axf::audio
