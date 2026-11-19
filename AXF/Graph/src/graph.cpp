#include "axf/graph/graph.hpp"

#include <algorithm>
#include <cstddef>
#include <deque>
#include <limits>
#include <string>

namespace axf::graph
{

const char* nodeKindName(NodeKind kind) noexcept
{
    switch (kind)
    {
    case NodeKind::AudioInput:
        return "AudioInput";
    case NodeKind::AudioOutput:
        return "AudioOutput";
    case NodeKind::Gain:
        return "Gain";
    case NodeKind::Pan:
        return "Pan";
    case NodeKind::Mixer:
        return "Mixer";
    case NodeKind::Bus:
        return "Bus";
    case NodeKind::Send:
        return "Send";
    case NodeKind::Return:
        return "Return";
    case NodeKind::DspProcessor:
        return "DspProcessor";
    case NodeKind::PluginProcessor:
        return "PluginProcessor";
    case NodeKind::Meter:
        return "Meter";
    case NodeKind::MidiProcessor:
        return "MidiProcessor";
    }
    return "Unknown";
}

const NodeDescriptor* Graph::findNode(NodeId id) const noexcept
{
    const auto it = std::find_if(nodes_.begin(), nodes_.end(),
                                 [id](const NodeDescriptor& node) { return node.id == id; });
    return it == nodes_.end() ? nullptr : &(*it);
}

core::Result<NodeId> Graph::addNode(NodeKind kind, std::string name, std::uint16_t audioInputs,
                                    std::uint16_t audioOutputs, std::uint16_t midiInputs,
                                    std::uint16_t midiOutputs, core::ChannelCount portChannels)
{
    if (nextId_ == kInvalidNodeId || nextId_ == std::numeric_limits<NodeId>::max())
    {
        return core::makeError(core::ErrorCode::OutOfRange, "node id space exhausted");
    }
    NodeDescriptor descriptor;
    descriptor.id = nextId_++;
    descriptor.kind = kind;
    descriptor.name = name.empty() ? nodeKindName(kind) : std::move(name);
    descriptor.audioInputs = audioInputs;
    descriptor.audioOutputs = audioOutputs;
    descriptor.midiInputs = midiInputs;
    descriptor.midiOutputs = midiOutputs;
    descriptor.portChannels = portChannels;
    nodes_.push_back(std::move(descriptor));
    return nodes_.back().id;
}

core::Result<void> Graph::removeNode(NodeId id)
{
    const auto it = std::find_if(nodes_.begin(), nodes_.end(),
                                 [id](const NodeDescriptor& node) { return node.id == id; });
    if (it == nodes_.end())
    {
        return core::makeError(core::ErrorCode::NotFound, "node not found");
    }
    nodes_.erase(it);
    connections_.erase(
        std::remove_if(connections_.begin(), connections_.end(), [id](const Connection& connection)
                       { return connection.from.node == id || connection.to.node == id; }),
        connections_.end());
    return core::success();
}

core::Result<void> Graph::connect(PortId from, PortId to, MediaType media)
{
    const NodeDescriptor* source = findNode(from.node);
    const NodeDescriptor* target = findNode(to.node);
    if (source == nullptr || target == nullptr)
    {
        return core::makeError(core::ErrorCode::NotFound, "connection endpoint not found");
    }
    if (from.node == to.node)
    {
        return core::makeError(core::ErrorCode::InvalidArgument, "self-connection is not allowed");
    }

    const std::uint16_t sourceCount =
        media == MediaType::Audio ? source->audioOutputs : source->midiOutputs;
    const std::uint16_t targetCount =
        media == MediaType::Audio ? target->audioInputs : target->midiInputs;
    if (from.index >= sourceCount || to.index >= targetCount)
    {
        return core::makeError(core::ErrorCode::OutOfRange, "port index out of range");
    }

    const Connection candidate{from, to, media};
    if (std::find(connections_.begin(), connections_.end(), candidate) != connections_.end())
    {
        return core::makeError(core::ErrorCode::AlreadyExists, "connection already exists");
    }

    connections_.push_back(candidate);
    const auto order = processingOrder();
    if (!order)
    {
        connections_.pop_back();
        return order.error();
    }
    return core::success();
}

core::Result<void> Graph::disconnect(PortId from, PortId to)
{
    const Connection candidate{from, to, MediaType::Audio};
    const auto it = std::find_if(
        connections_.begin(), connections_.end(), [&candidate](const Connection& connection)
        { return connection.from == candidate.from && connection.to == candidate.to; });
    if (it == connections_.end())
    {
        return core::makeError(core::ErrorCode::NotFound, "connection not found");
    }
    connections_.erase(it);
    return core::success();
}

core::Result<std::vector<NodeId>> Graph::processingOrder() const
{
    std::vector<std::size_t> indegree(nodes_.size(), 0U);
    std::vector<std::vector<std::size_t>> adjacency(nodes_.size());

    const auto indexOf = [this](NodeId id) -> std::size_t
    {
        for (std::size_t i = 0; i < nodes_.size(); ++i)
        {
            if (nodes_[i].id == id)
            {
                return i;
            }
        }
        return nodes_.size();
    };

    for (const Connection& connection : connections_)
    {
        const std::size_t from = indexOf(connection.from.node);
        const std::size_t to = indexOf(connection.to.node);
        if (from == nodes_.size() || to == nodes_.size())
        {
            return core::makeError(core::ErrorCode::InternalError,
                                   "connection references an unknown node");
        }
        adjacency[from].push_back(to);
        ++indegree[to];
    }

    std::deque<std::size_t> ready;
    for (std::size_t i = 0; i < nodes_.size(); ++i)
    {
        if (indegree[i] == 0)
        {
            ready.push_back(i);
        }
    }

    std::vector<NodeId> order;
    order.reserve(nodes_.size());
    while (!ready.empty())
    {
        const std::size_t current = ready.front();
        ready.pop_front();
        order.push_back(nodes_[current].id);
        for (const std::size_t next : adjacency[current])
        {
            if (--indegree[next] == 0)
            {
                ready.push_back(next);
            }
        }
    }

    if (order.size() != nodes_.size())
    {
        return core::makeError(core::ErrorCode::InvalidArgument, "graph contains a cycle");
    }
    return order;
}

bool Graph::hasCycle() const
{
    return !processingOrder().ok();
}

core::Result<void> Graph::validate() const
{
    for (const Connection& connection : connections_)
    {
        const NodeDescriptor* source = findNode(connection.from.node);
        const NodeDescriptor* target = findNode(connection.to.node);
        if (source == nullptr || target == nullptr)
        {
            return core::makeError(core::ErrorCode::NotFound,
                                   "connection references an unknown node");
        }
        const std::uint16_t sourceCount =
            connection.media == MediaType::Audio ? source->audioOutputs : source->midiOutputs;
        const std::uint16_t targetCount =
            connection.media == MediaType::Audio ? target->audioInputs : target->midiInputs;
        if (connection.from.index >= sourceCount || connection.to.index >= targetCount)
        {
            return core::makeError(core::ErrorCode::OutOfRange, "connection uses an invalid port");
        }
    }
    const auto order = processingOrder();
    if (!order)
    {
        return order.error();
    }
    return core::success();
}

void Graph::clear() noexcept
{
    nodes_.clear();
    connections_.clear();
    nextId_ = 1;
}

} // namespace axf::graph
