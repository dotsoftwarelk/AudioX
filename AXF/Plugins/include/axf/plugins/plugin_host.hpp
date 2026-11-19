#pragma once

#include "axf/audio/buffer.hpp"
#include "axf/core/result.hpp"
#include "axf/midi/event.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace axf::plugins
{

enum class PluginFormat : std::uint8_t
{
    Vst3
};

struct PluginDescriptor final
{
    std::string id;
    std::string name;
    std::string vendor;
    std::string version;
    std::string path;
    PluginFormat format{PluginFormat::Vst3};
};

/// Audio-thread interface. State and parameter operations are deliberately
/// excluded: all non-realtime work happens through the host on control thread.
class IPluginInstance
{
  public:
    virtual ~IPluginInstance() = default;
    virtual void prepare(double sampleRate, std::uint32_t maxFrames) noexcept = 0;
    virtual void reset() noexcept = 0;
    virtual void process(audio::AudioBufferView& audio,
                         midi::MidiEventBufferView& midi) noexcept = 0;
    [[nodiscard]] virtual std::uint32_t latencySamples() const noexcept = 0;
    [[nodiscard]] virtual core::Result<std::vector<std::byte>> getState() const = 0;
    [[nodiscard]] virtual core::Result<void> setState(const std::vector<std::byte>& state) = 0;
};

/// Scanning and loading live here, never in the audio callback.
/// Phase 5 supplies the VST3 implementation.
class IPluginHost
{
  public:
    virtual ~IPluginHost() = default;
    [[nodiscard]] virtual core::Result<std::vector<PluginDescriptor>> scan(
        const std::string& searchPath) = 0;
    [[nodiscard]] virtual core::Result<std::unique_ptr<IPluginInstance>> create(
        const PluginDescriptor& descriptor) = 0;
};

} // namespace axf::plugins
