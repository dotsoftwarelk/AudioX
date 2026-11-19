#include "axf/audio/compiled_graph.hpp"
#include "axf/core/rt.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <memory>
#include <utility>

namespace axf::audio
{
namespace
{
void clearChannel(float* data, std::uint32_t frames) noexcept
{
    if (data != nullptr && frames > 0)
    {
        std::memset(data, 0, static_cast<std::size_t>(frames) * sizeof(float));
    }
}

void copyChannel(const float* source, float* destination, std::uint32_t frames) noexcept
{
    if (source != nullptr && destination != nullptr && frames > 0)
    {
        std::memcpy(destination, source, static_cast<std::size_t>(frames) * sizeof(float));
    }
}

void mixChannel(const float* source, float* destination, std::uint32_t frames) noexcept
{
    if (source == nullptr || destination == nullptr)
    {
        return;
    }
    for (std::uint32_t frame = 0; frame < frames; ++frame)
    {
        destination[frame] += source[frame];
    }
}

bool isTerminalKind(graph::NodeKind kind) noexcept
{
    return kind == graph::NodeKind::AudioInput || kind == graph::NodeKind::AudioOutput;
}
} // namespace

CompiledGraph::CompiledGraph(EngineConfig config, std::vector<Node> nodes, std::vector<Edge> edges,
                             std::vector<std::unique_ptr<AudioBuffer>> buffers,
                             std::vector<std::unique_ptr<INodeProcessor>> processors,
                             GraphDiagnostics diagnostics) noexcept
    : config_(config), nodes_(std::move(nodes)), edges_(std::move(edges)),
      buffers_(std::move(buffers)), processors_(std::move(processors)), diagnostics_(diagnostics)
{
    for (std::size_t i = 0; i < nodes_.size(); ++i)
    {
        if (nodes_[i].kind == graph::NodeKind::AudioInput)
        {
            inputNodeIndex_ = i;
            break;
        }
    }
    for (std::size_t i = 0; i < nodes_.size(); ++i)
    {
        if (nodes_[i].kind == graph::NodeKind::AudioOutput)
        {
            outputNodeIndex_ = i;
            break;
        }
    }
}

void CompiledGraph::process(float* const* inputs, float* const* outputs, std::uint32_t frameCount,
                            std::uint64_t streamFrame) const noexcept
{
    core::rt::ScopedAudioThread guard;

    if (outputs == nullptr || frameCount == 0)
    {
        return;
    }
    if (frameCount > config_.maxFrames)
    {
        for (core::ChannelCount channel = 0; channel < config_.outputChannels; ++channel)
        {
            clearChannel(outputs[channel], frameCount);
        }
        return;
    }

    for (auto& buffer : buffers_)
    {
        buffer->clear();
    }

    const bool hasInputNode =
        !nodes_.empty() && nodes_[inputNodeIndex_].kind == graph::NodeKind::AudioInput;
    const bool hasOutputNode =
        !nodes_.empty() && nodes_[outputNodeIndex_].kind == graph::NodeKind::AudioOutput;

    if (hasInputNode)
    {
        const Node& inputNode = nodes_[inputNodeIndex_];
        AudioBuffer& inputBuffer = *buffers_[inputNode.outputBufferBase];
        for (core::ChannelCount channel = 0; channel < inputNode.channels; ++channel)
        {
            if (channel < config_.inputChannels && inputs != nullptr)
            {
                copyChannel(inputs[channel], inputBuffer.channelData(channel), frameCount);
            }
            else
            {
                clearChannel(inputBuffer.channelData(channel), frameCount);
            }
        }
    }

    std::array<AudioBufferView, kMaxNodePorts> inputViews{};
    std::array<AudioBufferView, kMaxNodePorts> outputViews{};

    for (std::size_t nodeIndex = 0; nodeIndex < nodes_.size(); ++nodeIndex)
    {
        const Node& node = nodes_[nodeIndex];
        const std::uint16_t inputCount =
            static_cast<std::uint16_t>(std::min<std::size_t>(node.inputPorts, kMaxNodePorts));
        const std::uint16_t outputCount =
            static_cast<std::uint16_t>(std::min<std::size_t>(node.outputPorts, kMaxNodePorts));

        for (std::uint16_t port = 0; port < inputCount; ++port)
        {
            inputViews[port] = makeView(*buffers_[node.inputBufferBase + port]);
        }
        for (std::uint16_t port = 0; port < outputCount; ++port)
        {
            outputViews[port] = makeView(*buffers_[node.outputBufferBase + port]);
        }

        for (const Edge& edge : edges_)
        {
            if (edge.toNode != nodeIndex)
            {
                continue;
            }
            const Node& source = nodes_[edge.fromNode];
            const AudioBuffer& from = *buffers_[source.outputBufferBase + edge.fromPort];
            AudioBuffer& to = *buffers_[node.inputBufferBase + edge.toPort];
            const core::ChannelCount channels = std::min(from.channels(), to.channels());
            for (core::ChannelCount channel = 0; channel < channels; ++channel)
            {
                mixChannel(from.channelData(channel), to.channelData(channel), frameCount);
            }
        }

        if (node.processor != nullptr)
        {
            ProcessContext context{inputViews.data(),  inputCount,
                                   outputViews.data(), outputCount,
                                   frameCount,         static_cast<double>(config_.sampleRate),
                                   streamFrame};
            node.processor->process(context);
        }
    }

    if (hasOutputNode)
    {
        const Node& outputNode = nodes_[outputNodeIndex_];
        const AudioBuffer& outputBuffer = *buffers_[outputNode.inputBufferBase];
        for (core::ChannelCount channel = 0; channel < config_.outputChannels; ++channel)
        {
            if (channel < outputNode.channels)
            {
                copyChannel(outputBuffer.channelData(channel), outputs[channel], frameCount);
            }
            else
            {
                clearChannel(outputs[channel], frameCount);
            }
        }
    }
    else
    {
        for (core::ChannelCount channel = 0; channel < config_.outputChannels; ++channel)
        {
            clearChannel(outputs[channel], frameCount);
        }
    }
}

core::Result<std::unique_ptr<CompiledGraph>> compileGraph(
    const graph::Graph& description, const EngineConfig& config,
    std::map<graph::NodeId, std::unique_ptr<INodeProcessor>>& processorOverrides)
{
    const auto validation = description.validate();
    if (!validation)
    {
        return validation.error();
    }
    if (!core::isValidSampleRate(config.sampleRate) || config.maxFrames == 0 ||
        config.outputChannels == 0)
    {
        return core::makeError(core::ErrorCode::InvalidArgument, "invalid engine configuration");
    }

    const auto orderResult = description.processingOrder();
    if (!orderResult)
    {
        return orderResult.error();
    }
    const std::vector<graph::NodeId> order = orderResult.value();
    const core::ChannelCount portChannels =
        config.inputChannels > config.outputChannels ? config.inputChannels : config.outputChannels;

    std::vector<CompiledGraph::Node> nodes;
    std::vector<std::unique_ptr<AudioBuffer>> buffers;
    std::vector<std::unique_ptr<INodeProcessor>> processors;
    nodes.reserve(order.size());
    buffers.reserve(order.size() * kMaxNodePorts);
    processors.reserve(order.size());

    for (const graph::NodeId id : order)
    {
        const graph::NodeDescriptor* descriptor = description.findNode(id);
        if (descriptor == nullptr)
        {
            return core::makeError(core::ErrorCode::InternalError, "missing node descriptor");
        }
        if (descriptor->portChannels != portChannels)
        {
            return core::makeError(core::ErrorCode::Unsupported,
                                   "node port channel count does not match engine configuration");
        }
        if (descriptor->audioInputs > kMaxNodePorts || descriptor->audioOutputs > kMaxNodePorts)
        {
            return core::makeError(core::ErrorCode::OutOfRange,
                                   "node port count exceeds engine limit");
        }

        CompiledGraph::Node node;
        node.id = id;
        node.kind = descriptor->kind;
        node.channels = descriptor->portChannels;
        node.inputPorts = descriptor->audioInputs;
        node.outputPorts = descriptor->audioOutputs;

        node.inputBufferBase = buffers.size();
        for (std::uint16_t port = 0; port < node.inputPorts; ++port)
        {
            buffers.push_back(std::make_unique<AudioBuffer>(node.channels, config.maxFrames));
        }
        node.outputBufferBase = buffers.size();
        for (std::uint16_t port = 0; port < node.outputPorts; ++port)
        {
            buffers.push_back(std::make_unique<AudioBuffer>(node.channels, config.maxFrames));
        }

        if (!isTerminalKind(node.kind))
        {
            auto overrideIt = processorOverrides.find(id);
            std::unique_ptr<INodeProcessor> processor =
                overrideIt == processorOverrides.end() ? std::make_unique<PassThroughProcessor>()
                                                       : std::move(overrideIt->second);
            if (overrideIt != processorOverrides.end())
            {
                processorOverrides.erase(overrideIt);
            }
            processor->prepare(static_cast<double>(config.sampleRate), config.maxFrames);
            node.processor = processor.get();
            node.latencySamples = processor->latencySamples();
            processors.push_back(std::move(processor));
        }
        nodes.push_back(node);
    }

    std::map<graph::NodeId, std::size_t> indexOf;
    for (std::size_t i = 0; i < nodes.size(); ++i)
    {
        indexOf[nodes[i].id] = i;
    }

    std::vector<CompiledGraph::Edge> edges;
    GraphDiagnostics diagnostics;
    diagnostics.nodeCount = nodes.size();
    for (const graph::Connection& connection : description.connections())
    {
        if (connection.media != graph::MediaType::Audio)
        {
            ++diagnostics.midiConnectionCount;
            continue;
        }
        CompiledGraph::Edge edge;
        edge.fromNode = indexOf[connection.from.node];
        edge.fromPort = connection.from.index;
        edge.toNode = indexOf[connection.to.node];
        edge.toPort = connection.to.index;
        edges.push_back(edge);
        ++diagnostics.audioConnectionCount;
    }

    std::vector<std::uint32_t> accumulated(nodes.size(), 0);
    for (std::size_t i = 0; i < nodes.size(); ++i)
    {
        std::uint32_t upstream = 0;
        for (const CompiledGraph::Edge& edge : edges)
        {
            if (edge.toNode == i)
            {
                upstream = std::max(upstream, accumulated[edge.fromNode]);
            }
        }
        accumulated[i] = upstream + nodes[i].latencySamples;
        if (nodes[i].kind == graph::NodeKind::AudioOutput)
        {
            diagnostics.latencySamples = std::max(diagnostics.latencySamples, accumulated[i]);
        }
    }

    diagnostics.valid = true;
    diagnostics.error = "ok";
    return std::make_unique<CompiledGraph>(config, std::move(nodes), std::move(edges),
                                           std::move(buffers), std::move(processors), diagnostics);
}

} // namespace axf::audio
