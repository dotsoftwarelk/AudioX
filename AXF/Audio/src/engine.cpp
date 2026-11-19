#include "axf/audio/engine.hpp"
#include "axf/core/rt.hpp"

#include <atomic>
#include <cstring>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

namespace axf::audio
{

struct AudioEngine::Impl final
{
    explicit Impl(EngineConfig engineConfig) : config(engineConfig)
    {
    }

    EngineConfig config;
    std::mutex controlMutex;
    graph::Graph pendingDescription;
    bool hasPendingDescription{false};
    std::map<graph::NodeId, std::unique_ptr<INodeProcessor>> processorOverrides;
    std::atomic<const CompiledGraph*> active{nullptr};
    std::vector<std::unique_ptr<CompiledGraph>> history;
    GraphDiagnostics lastDiagnostics;
};

AudioEngine::AudioEngine(EngineConfig config) : impl_(std::make_unique<Impl>(config))
{
}

AudioEngine::~AudioEngine() = default;

core::Result<void> AudioEngine::setGraph(const graph::Graph& description)
{
    std::lock_guard<std::mutex> lock(impl_->controlMutex);
    impl_->pendingDescription = description;
    impl_->hasPendingDescription = true;
    return core::success();
}

void AudioEngine::setProcessor(graph::NodeId nodeId, std::unique_ptr<INodeProcessor> processor)
{
    std::lock_guard<std::mutex> lock(impl_->controlMutex);
    impl_->processorOverrides[nodeId] = std::move(processor);
}

core::Result<void> AudioEngine::commit()
{
    std::lock_guard<std::mutex> lock(impl_->controlMutex);
    if (!impl_->hasPendingDescription)
    {
        return core::makeError(core::ErrorCode::InvalidArgument, "no graph description to commit");
    }

    auto compiled =
        compileGraph(impl_->pendingDescription, impl_->config, impl_->processorOverrides);
    if (!compiled)
    {
        return compiled.error();
    }

    impl_->history.push_back(std::move(compiled.value()));
    impl_->lastDiagnostics = impl_->history.back()->diagnostics();
    impl_->active.store(impl_->history.back().get(), std::memory_order_release);
    return core::success();
}

void AudioEngine::process(float* const* inputs, float* const* outputs, std::uint32_t frameCount,
                          std::uint64_t streamFrame) noexcept
{
    core::rt::ScopedAudioThread guard;
    const CompiledGraph* graph = impl_->active.load(std::memory_order_acquire);
    if (graph != nullptr)
    {
        graph->process(inputs, outputs, frameCount, streamFrame);
        return;
    }

    if (outputs == nullptr)
    {
        return;
    }
    for (core::ChannelCount channel = 0; channel < impl_->config.outputChannels; ++channel)
    {
        if (outputs[channel] != nullptr && frameCount > 0)
        {
            std::memset(outputs[channel], 0, static_cast<std::size_t>(frameCount) * sizeof(float));
        }
    }
}

const CompiledGraph* AudioEngine::activeGraph() const noexcept
{
    return impl_->active.load(std::memory_order_acquire);
}

GraphDiagnostics AudioEngine::diagnostics() const noexcept
{
    std::lock_guard<std::mutex> lock(impl_->controlMutex);
    return impl_->lastDiagnostics;
}

std::uint32_t AudioEngine::latencySamples() const noexcept
{
    return impl_->active.load(std::memory_order_acquire) == nullptr
               ? 0
               : impl_->active.load(std::memory_order_acquire)->diagnostics().latencySamples;
}

EngineConfig AudioEngine::config() const noexcept
{
    return impl_->config;
}

} // namespace axf::audio
