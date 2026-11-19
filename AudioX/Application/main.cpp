#include "axf/audio/buffer.hpp"
#include "axf/audio/clock.hpp"
#include "axf/audio/transport.hpp"
#include "axf/core/error.hpp"
#include "axf/core/types.hpp"
#include "axf/devices/device.hpp"
#include "axf/dsp/utility.hpp"
#include "axf/graph/graph.hpp"
#include "axf/media/audio_file.hpp"
#include "axf/platform/platform.hpp"
#include "axf/project/project.hpp"

#include <cstring>
#include <iostream>
#include <string>
#include <string_view>

namespace
{

int printInfo()
{
    std::cout << "AudioX DAW shell\n"
              << "  AXF version:    " << axf::platform::frameworkVersion() << '\n'
              << "  Host OS:        " << axf::platform::osName() << '\n'
              << "  Project format: " << axf::project::projectFormatVersion() << '\n'
              << "  Default backend:"
              << axf::devices::backendName(axf::devices::Backend::WasapiShared) << '\n';
    return 0;
}

int selfTest()
{
    using namespace axf;

    audio::AudioBuffer buffer(2, 64);
    buffer.channelData(0)[0] = 1.0F;
    buffer.channelData(1)[0] = -0.5F;
    auto view = audio::makeView(buffer);
    dsp::applyGain(view, 0.5F);
    if (buffer.channelData(0)[0] != 0.5F || buffer.channelData(1)[0] != -0.25F)
    {
        std::cerr << "self-test failed: gain stage\n";
        return 1;
    }

    graph::Graph graph;
    const auto input = graph.addNode(graph::NodeKind::AudioInput, "Input", 0, 1);
    const auto gain = graph.addNode(graph::NodeKind::Gain, "Gain", 1, 1);
    const auto output = graph.addNode(graph::NodeKind::AudioOutput, "Output", 1, 0);
    if (!input || !gain || !output)
    {
        std::cerr << "self-test failed: addNode\n";
        return 1;
    }
    if (!graph.connect({input.value(), 0}, {gain.value(), 0}, graph::MediaType::Audio) ||
        !graph.connect({gain.value(), 0}, {output.value(), 0}, graph::MediaType::Audio))
    {
        std::cerr << "self-test failed: connect\n";
        return 1;
    }
    if (!graph.validate())
    {
        std::cerr << "self-test failed: graph validation: "
                  << core::formatError(graph.validate().error()) << '\n';
        return 1;
    }

    audio::Transport transport(48000);
    transport.play();
    audio::advanceTransport(transport, 128);
    if (transport.position() != 128 || !transport.isRolling())
    {
        std::cerr << "self-test failed: transport\n";
        return 1;
    }
    transport.stop();

    std::cout << "self-test: PASS (" << media::formatName(media::formatFromExtension("mixdown.wav"))
              << ", " << core::kAxfVersion.major << '.' << core::kAxfVersion.minor << ")\n";
    return 0;
}

} // namespace

int main(int argc, char** argv)
{
    enum class Action
    {
        Info,
        SelfTest,
        Help,
    };

    Action action = Action::Info;
    for (int i = 1; i < argc; ++i)
    {
        const std::string_view argument{argv[i]};
        if (argument == "--self-test")
        {
            action = Action::SelfTest;
        }
        else if (argument == "--info")
        {
            action = Action::Info;
        }
        else if (argument == "--help" || argument == "-h")
        {
            action = Action::Help;
        }
        else
        {
            std::cerr << "unknown argument: " << argument << '\n';
            return 2;
        }
    }

    switch (action)
    {
    case Action::SelfTest:
        return selfTest();
    case Action::Info:
        return printInfo();
    case Action::Help:
        std::cout << "usage: audiox [--info] [--self-test] [--help]\n";
        return 0;
    }
    return 0;
}
