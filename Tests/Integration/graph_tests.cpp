#include <gtest/gtest.h>

#include "axf/graph/graph.hpp"

#include <algorithm>

using namespace axf;

namespace
{
std::vector<graph::NodeId> orderIds(const graph::Graph& graph)
{
    const auto order = graph.processingOrder();
    return order ? order.value() : std::vector<graph::NodeId>{};
}
} // namespace

TEST(Graph, ValidChainProcessingOrder)
{
    graph::Graph graph;
    const auto input = graph.addNode(graph::NodeKind::AudioInput, "in", 0, 1);
    const auto gain = graph.addNode(graph::NodeKind::Gain, "gain", 1, 1);
    const auto output = graph.addNode(graph::NodeKind::AudioOutput, "out", 1, 0);
    ASSERT_TRUE(input && gain && output);
    ASSERT_TRUE(graph.connect({input.value(), 0}, {gain.value(), 0}, graph::MediaType::Audio).ok());
    ASSERT_TRUE(
        graph.connect({gain.value(), 0}, {output.value(), 0}, graph::MediaType::Audio).ok());
    EXPECT_TRUE(graph.validate().ok());
    EXPECT_FALSE(graph.hasCycle());
    const auto order = orderIds(graph);
    ASSERT_EQ(order.size(), 3U);
    EXPECT_EQ(order[0], input.value());
    EXPECT_EQ(order[2], output.value());
    EXPECT_STREQ(graph::nodeKindName(graph::NodeKind::PluginProcessor), "PluginProcessor");
}

TEST(Graph, RejectsInvalidAndDuplicateConnections)
{
    graph::Graph graph;
    const auto gain = graph.addNode(graph::NodeKind::Gain, "gain", 1, 1);
    const auto meter = graph.addNode(graph::NodeKind::Meter, "meter", 1, 0);
    ASSERT_TRUE(gain && meter);

    EXPECT_EQ(
        graph.connect({gain.value(), 0}, {gain.value(), 0}, graph::MediaType::Audio).error().code,
        core::ErrorCode::InvalidArgument);
    EXPECT_EQ(
        graph.connect({gain.value(), 4}, {meter.value(), 0}, graph::MediaType::Audio).error().code,
        core::ErrorCode::OutOfRange);
    ASSERT_TRUE(graph.connect({gain.value(), 0}, {meter.value(), 0}, graph::MediaType::Audio).ok());
    EXPECT_EQ(
        graph.connect({gain.value(), 0}, {meter.value(), 0}, graph::MediaType::Audio).error().code,
        core::ErrorCode::AlreadyExists);
}

TEST(Graph, RejectsFeedbackCycle)
{
    graph::Graph graph;
    const auto first = graph.addNode(graph::NodeKind::Gain, "a", 1, 1);
    const auto second = graph.addNode(graph::NodeKind::Gain, "b", 1, 1);
    ASSERT_TRUE(first && second);
    ASSERT_TRUE(
        graph.connect({first.value(), 0}, {second.value(), 0}, graph::MediaType::Audio).ok());
    const auto feedback =
        graph.connect({second.value(), 0}, {first.value(), 0}, graph::MediaType::Audio);
    EXPECT_FALSE(feedback.ok());
    EXPECT_NE(std::string(feedback.error().message).find("cycle"), std::string::npos);
    EXPECT_EQ(graph.connections().size(), 1U);
}

TEST(Graph, RemovingNodeRemovesAttachedConnections)
{
    graph::Graph graph;
    const auto source = graph.addNode(graph::NodeKind::Gain, "a", 0, 1);
    const auto sink = graph.addNode(graph::NodeKind::Meter, "b", 1, 0);
    ASSERT_TRUE(source && sink);
    ASSERT_TRUE(
        graph.connect({source.value(), 0}, {sink.value(), 0}, graph::MediaType::Audio).ok());
    EXPECT_TRUE(graph.removeNode(source.value()).ok());
    EXPECT_TRUE(graph.connections().empty());
    EXPECT_EQ(graph.findNode(source.value()), nullptr);
    EXPECT_FALSE(graph.removeNode(source.value()).ok());
}

TEST(Graph, MidiPortsAreValidatedSeparately)
{
    graph::Graph graph;
    const auto processor = graph.addNode(graph::NodeKind::MidiProcessor, "midi", 0, 0, 1, 1);
    const auto plugin = graph.addNode(graph::NodeKind::PluginProcessor, "vst", 0, 0, 1, 0);
    ASSERT_TRUE(processor && plugin);
    EXPECT_TRUE(
        graph.connect({processor.value(), 0}, {plugin.value(), 0}, graph::MediaType::Midi).ok());
    EXPECT_TRUE(graph.validate().ok());
}
