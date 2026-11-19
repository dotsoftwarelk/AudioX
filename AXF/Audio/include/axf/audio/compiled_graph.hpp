#pragma once

#include "axf/audio/buffer.hpp"
#include "axf/audio/node.hpp"
#include "axf/core/export.hpp"
#include "axf/core/result.hpp"
#include "axf/graph/graph.hpp"

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <vector>

namespace axf::audio
{

struct EngineConfig final
{
    core::SampleRate sampleRate{48000};
    std::uint32_t maxFrames{512};
    core::ChannelCount inputChannels{2};
    core::ChannelCount outputChannels{2};
};

struct GraphDiagnostics final
{
    std::size_t nodeCount{0};
    std::size_t audioConnectionCount{0};
    std::size_t midiConnectionCount{0};
    std::uint32_t latencySamples{0};
    bool valid{false};
    const char* error{"not compiled"};
};

/// Immutable, preallocated processing plan. Built on the control thread, then
/// published to the audio thread. Never mutate it after publication.
class CompiledGraph final
{
  public:
    struct Node final
    {
        graph::NodeId id{graph::kInvalidNodeId};
        graph::NodeKind kind{graph::NodeKind::Gain};
        core::ChannelCount channels{2};
        std::uint16_t inputPorts{0};
        std::uint16_t outputPorts{0};
        std::uint32_t latencySamples{0};
        INodeProcessor* processor{nullptr};
        std::size_t inputBufferBase{0};
        std::size_t outputBufferBase{0};
    };

    struct Edge final
    {
        std::size_t fromNode{0};
        std::uint16_t fromPort{0};
        std::size_t toNode{0};
        std::uint16_t toPort{0};
    };

    CompiledGraph(EngineConfig config, std::vector<Node> nodes, std::vector<Edge> edges,
                  std::vector<std::unique_ptr<AudioBuffer>> buffers,
                  std::vector<std::unique_ptr<INodeProcessor>> processors,
                  GraphDiagnostics diagnostics) noexcept;

    /// Audio-thread entry point. Satisfies the AXF real-time contract.
    void process(float* const* inputs, float* const* outputs, std::uint32_t frameCount,
                 std::uint64_t streamFrame) const noexcept;

    [[nodiscard]] const std::vector<Node>& nodes() const noexcept
    {
        return nodes_;
    }
    [[nodiscard]] const std::vector<Edge>& edges() const noexcept
    {
        return edges_;
    }
    [[nodiscard]] const GraphDiagnostics& diagnostics() const noexcept
    {
        return diagnostics_;
    }
    [[nodiscard]] EngineConfig config() const noexcept
    {
        return config_;
    }

  private:
    EngineConfig config_;
    std::vector<Node> nodes_;
    std::vector<Edge> edges_;
    std::vector<std::unique_ptr<AudioBuffer>> buffers_;
    std::vector<std::unique_ptr<INodeProcessor>> processors_;
    GraphDiagnostics diagnostics_;
    std::size_t inputNodeIndex_{0};
    std::size_t outputNodeIndex_{0};
};

/// Builds an immutable graph from a control-thread description. Processor
/// overrides are keyed by node ID; unspecified nodes get a pass-through
/// processor. No audio-thread work is performed here.
[[nodiscard]] AXF_API core::Result<std::unique_ptr<CompiledGraph>> compileGraph(
    const graph::Graph& description, const EngineConfig& config,
    std::map<graph::NodeId, std::unique_ptr<INodeProcessor>>& processorOverrides);

} // namespace axf::audio
