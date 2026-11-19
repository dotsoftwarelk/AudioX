#include "axf/core/rt.hpp"

#include <atomic>

namespace axf::core::rt
{
namespace
{
thread_local bool t_audioThread = false;
std::atomic<AllocationHook> g_hook{nullptr};
std::atomic<void*> g_userData{nullptr};
std::atomic<std::uint64_t> g_violations{0};

void invoke(std::size_t bytes, bool deallocate) noexcept
{
    if (!t_audioThread)
    {
        return;
    }
    if (!deallocate)
    {
        g_violations.fetch_add(1, std::memory_order_relaxed);
    }
    const AllocationHook hook = g_hook.load(std::memory_order_acquire);
    if (hook != nullptr)
    {
        hook(bytes, deallocate, g_userData.load(std::memory_order_relaxed));
    }
}
} // namespace

void markAudioThread() noexcept
{
    t_audioThread = true;
}

void clearAudioThread() noexcept
{
    t_audioThread = false;
}

bool isAudioThread() noexcept
{
    return t_audioThread;
}

ScopedAudioThread::ScopedAudioThread() noexcept : previous_(t_audioThread)
{
    t_audioThread = true;
}

ScopedAudioThread::~ScopedAudioThread() noexcept
{
    t_audioThread = previous_;
}

void setAllocationHook(AllocationHook hook, void* userData) noexcept
{
    g_userData.store(userData, std::memory_order_relaxed);
    g_hook.store(hook, std::memory_order_release);
}

void noteAllocation(std::size_t bytes, bool deallocate) noexcept
{
    invoke(bytes, deallocate);
}

void noteRealtimeViolation() noexcept
{
    invoke(0, false);
}

std::uint64_t violationCount() noexcept
{
    return g_violations.load(std::memory_order_relaxed);
}

void resetViolations() noexcept
{
    g_violations.store(0, std::memory_order_relaxed);
}

} // namespace axf::core::rt
