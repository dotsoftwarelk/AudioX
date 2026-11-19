#include "axf/core/memory.hpp"
#include "axf/core/types.hpp"

#include <cstring>
#include <new>

namespace axf::core
{
namespace
{
std::size_t effectiveAlignment(std::size_t requested) noexcept
{
    return requested > 0 && isPowerOfTwo(requested) ? requested : alignof(std::max_align_t);
}
} // namespace

AlignedBuffer::AlignedBuffer(std::size_t bytes, std::size_t alignment)
    : size_(bytes), alignment_(effectiveAlignment(alignment))
{
    if (size_ == 0)
    {
        return;
    }
    data_ = ::operator new(size_, std::align_val_t{alignment_});
}

AlignedBuffer::~AlignedBuffer()
{
    if (data_ != nullptr)
    {
        ::operator delete(data_, std::align_val_t{alignment_});
    }
}

void AlignedBuffer::clear() noexcept
{
    if (data_ != nullptr)
    {
        std::memset(data_, 0, size_);
    }
}

MonotonicPool::MonotonicPool(std::size_t blockSize, std::size_t blockCount)
    : blockCount_(blockCount)
{
    const std::size_t alignment = alignof(std::max_align_t);
    stride_ = blockSize == 0 ? alignment : ((blockSize + alignment - 1) / alignment) * alignment;
    if (blockCount_ > 0)
    {
        storage_ = std::make_unique<std::byte[]>(stride_ * blockCount_);
    }
}

MonotonicPool::~MonotonicPool() = default;

void* MonotonicPool::allocate() noexcept
{
    if (nextIndex_ >= blockCount_ || !storage_)
    {
        return nullptr;
    }
    void* result = storage_.get() + nextIndex_ * stride_;
    ++nextIndex_;
    return result;
}

} // namespace axf::core
