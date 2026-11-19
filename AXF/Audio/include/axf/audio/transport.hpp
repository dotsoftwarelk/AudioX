#pragma once

#include "axf/core/export.hpp"
#include "axf/core/types.hpp"

#include <atomic>

namespace axf::audio
{

class Transport final
{
  public:
    enum class State : std::uint8_t
    {
        Stopped,
        Playing,
        Paused,
        Recording
    };

    explicit Transport(core::SampleRate sampleRate) noexcept
        : sampleRate_(sampleRate > 0 ? sampleRate : 48000), state_(State::Stopped), position_(0)
    {
    }

    void play() noexcept
    {
        state_.store(State::Playing, std::memory_order_release);
    }
    void pause() noexcept
    {
        state_.store(State::Paused, std::memory_order_release);
    }
    void record() noexcept
    {
        state_.store(State::Recording, std::memory_order_release);
    }
    void stop() noexcept
    {
        state_.store(State::Stopped, std::memory_order_release);
    }

    [[nodiscard]] State state() const noexcept
    {
        return state_.load(std::memory_order_acquire);
    }
    [[nodiscard]] bool isRolling() const noexcept
    {
        const State s = state();
        return s == State::Playing || s == State::Recording;
    }
    [[nodiscard]] core::FrameIndex position() const noexcept
    {
        return position_.load(std::memory_order_relaxed);
    }
    void setPosition(core::FrameIndex frame) noexcept
    {
        position_.store(frame < 0 ? 0 : frame, std::memory_order_relaxed);
    }
    [[nodiscard]] core::SampleRate sampleRate() const noexcept
    {
        return sampleRate_.load(std::memory_order_relaxed);
    }
    void setSampleRate(core::SampleRate rate) noexcept
    {
        sampleRate_.store(rate > 0 ? rate : 48000, std::memory_order_relaxed);
    }

    struct Snapshot final
    {
        State state;
        core::FrameIndex position;
        core::SampleRate sampleRate;
    };

    /// Individual fields are atomic but the snapshot is not a consistent
    /// point-in-time view; stop the transport before persisting it.
    [[nodiscard]] Snapshot snapshot() const noexcept
    {
        return Snapshot{state(), position(), sampleRate()};
    }

  private:
    std::atomic<core::SampleRate> sampleRate_;
    std::atomic<State> state_;
    std::atomic<core::FrameIndex> position_;
};

AXF_API void advanceTransport(Transport& transport, core::FrameIndex frames) noexcept;
[[nodiscard]] AXF_API const char* transportStateName(Transport::State state) noexcept;

} // namespace axf::audio
