#pragma once

#include "axf/core/export.hpp"
#include "axf/core/result.hpp"
#include "axf/core/types.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace axf::graph
{

enum class NodeKind : std::uint8_t
{
    AudioInput,
    AudioOutput,
    Gain,
    Pan,
    Mixer,
    Bus,
    Send,
    Return,
    DspProcessor,
    PluginProcessor,
    Meter,
    MidiProcessor,
};

[[nodiscard]] AXF_API const char* nodeKindName(NodeKind kind) noexcept;

using NodeId = std::uint32_t;
constexpr NodeId kInvalidNodeId = 0;

enum class PortDirection : std::uint8_t
{
    Input,
    Output
};
enum class MediaType : std::uint8_t
{
    Audio,
    Midi
};

struct PortId final
{
    NodeId node{kInvalidNodeId};
    std::uint16_t index{0};

    friend bool operator==(const PortId&, const PortId&) noexcept = default;
};

struct Connection final
{
    PortId from; ///< Must be an output port.
    PortId to;   ///< Must be an input port.
    MediaType media{MediaType::Audio};

    friend bool operator==(const Connection&, const Connection&) noexcept = default;
};

struct NodeDescriptor final
{
    NodeId id{kInvalidNodeId};
    NodeKind kind{NodeKind::Gain};
    std::string name;
    std::uint16_t audioInputs{0};
    std::uint16_t audioOutputs{0};
    std::uint16_t midiInputs{0};
    std::uint16_t midiOutputs{0};
    core::ChannelCount portChannels{2};
};

/// Control-thread graph description. There is deliberately no locking here:
/// the audio engine consumes an immutable compiled snapshot built from this
/// description (implemented in Phase 2). Never mutate from the audio callback.
class Graph final
{
  public:
    Graph() = default;

    [[nodiscard]] core::Result<NodeId> addNode(NodeKind kind, std::string name,
                                               std::uint16_t audioInputs,
                                               std::uint16_t audioOutputs,
                                               std::uint16_t midiInputs = 0,
                                               std::uint16_t midiOutputs = 0,
                                               core::ChannelCount portChannels = 2);
    [[nodiscard]] core::Result<void> removeNode(NodeId id);
    [[nodiscard]] core::Result<void> connect(PortId from, PortId to, MediaType media);
    [[nodiscard]] core::Result<void> disconnect(PortId from, PortId to);

    [[nodiscard]] const std::vector<NodeDescriptor>& nodes() const noexcept
    {
        return nodes_;
    }
    [[nodiscard]] const std::vector<Connection>& connections() const noexcept
    {
        return connections_;
    }

    [[nodiscard]] const NodeDescriptor* findNode(NodeId id) const noexcept;

    [[nodiscard]] core::Result<std::vector<NodeId>> processingOrder() const;
    [[nodiscard]] bool hasCycle() const;
    [[nodiscard]] core::Result<void> validate() const;

    void clear() noexcept;

  private:
    std::vector<NodeDescriptor> nodes_;
    std::vector<Connection> connections_;
    NodeId nextId_{1};
};

} // namespace axf::graph
