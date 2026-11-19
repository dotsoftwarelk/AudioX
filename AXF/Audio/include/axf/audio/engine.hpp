#pragma once

#include "axf/audio/compiled_graph.hpp"
#include "axf/core/export.hpp"
#include "axf/core/result.hpp"

#include <map>
#include <memory>

namespace axf::audio
{

/// Host-facing audio engine. Control-thread configuration is separated from
/// the audio-thread process() call by an atomically published graph pointer.
class AudioEngine final
{
  public:
    explicit AudioEngine(EngineConfig config);
    ~AudioEngine();

    AudioEngine(const AudioEngine&) = delete;
    AudioEngine& operator=(const AudioEngine&) = delete;

    /// Compiles a pending graph. Nothing changes on the audio thread until
    /// commit() is called.
    [[nodiscard]] core::Result<void> setGraph(const graph::Graph& description);

    /// Installs a processor for a node before commit().
    void setProcessor(graph::NodeId nodeId, std::unique_ptr<INodeProcessor> processor);

    /// Publishes the pending graph at the next process() call. Old graphs are
    /// retained until engine destruction for safe reclamation.
    [[nodiscard]] core::Result<void> commit();

    /// Audio-thread callback substitute for Phase 2. Device backends call this
    /// with preallocated device buffers in Phase 3.
    void process(float* const* inputs, float* const* outputs, std::uint32_t frameCount,
                 std::uint64_t streamFrame) noexcept;

    [[nodiscard]] const CompiledGraph* activeGraph() const noexcept;
    [[nodiscard]] GraphDiagnostics diagnostics() const noexcept;
    [[nodiscard]] std::uint32_t latencySamples() const noexcept;
    [[nodiscard]] EngineConfig config() const noexcept;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace axf::audio
