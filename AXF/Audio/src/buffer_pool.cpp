#include "axf/audio/buffer_pool.hpp"

namespace axf::audio
{

AudioBufferPool::AudioBufferPool(core::ChannelCount channels, std::uint32_t frames,
                                 std::size_t count)
{
    buffers_.reserve(count);
    for (std::size_t i = 0; i < count; ++i)
    {
        buffers_.push_back(
            std::make_unique<AudioBuffer>(channels, static_cast<std::size_t>(frames)));
    }
}

} // namespace axf::audio
