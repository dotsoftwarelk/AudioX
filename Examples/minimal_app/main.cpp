#include "axf/audio/buffer.hpp"
#include "axf/dsp/utility.hpp"
#include "axf/graph/graph.hpp"

#include <iostream>

int main()
{
    axf::audio::AudioBuffer buffer(2, 32);
    buffer.fill(1.0F);
    auto view = axf::audio::makeView(buffer);
    axf::dsp::applyGain(view, 0.5F);

    axf::graph::Graph graph;
    const auto source = graph.addNode(axf::graph::NodeKind::AudioInput, "in", 0, 1);
    const auto sink = graph.addNode(axf::graph::NodeKind::AudioOutput, "out", 1, 0);
    if (!source || !sink || !graph.validate())
    {
        return 1;
    }
    std::cout << "AXF standalone consumer OK; first sample=" << buffer.channelData(0)[0] << '\n';
    return 0;
}
