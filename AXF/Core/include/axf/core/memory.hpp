#pragma once

#include "axf/core/export.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>

namespace axf::core
{

/// Owns one aligned, preallocated memory block. Construct on a control thread;
/// data() may be used from the audio thread without locking or allocating.
class AlignedBuffer final
{
  public:
    AlignedBuffer(std::size_t bytes, std::size_t alignment = 64);
    ~AlignedBuffer();

    AlignedBuffer(const AlignedBuffer&) = delete;
    AlignedBuffer& operator=(const AlignedBuffer&) = delete;

    [[nodiscard]] void* data() noexcept
    {
        return data_;
    }
    [[nodiscard]] const void* data() const noexcept
    {
        return data_;
    }
    [[nodiscard]] std::size_t size() const noexcept
    {
        return size_;
    }
    [[nodiscard]] std::size_t alignment() const noexcept
    {
        return alignment_;
    }
    void clear() noexcept;

  private:
    void* data_{nullptr};
    std::size_t size_{0};
    std::size_t alignment_{64};
};

/// Bump allocator over one preallocated block. allocate() never allocates
/// system memory. reset() is only valid when no block is still in use.
class MonotonicPool final
{
  public:
    MonotonicPool(std::size_t blockSize, std::size_t blockCount);
    ~MonotonicPool();

    MonotonicPool(const MonotonicPool&) = delete;
    MonotonicPool& operator=(const MonotonicPool&) = delete;

    [[nodiscard]] void* allocate() noexcept;

    template <class T> [[nodiscard]] T* allocateAs() noexcept
    {
        static_assert(alignof(T) <= alignof(std::max_align_t),
                      "pool blocks must satisfy the requested alignment");
        return static_cast<T*>(allocate());
    }

    void reset() noexcept
    {
        nextIndex_ = 0;
    }

    [[nodiscard]] std::size_t capacity() const noexcept
    {
        return blockCount_;
    }
    [[nodiscard]] std::size_t available() const noexcept
    {
        return blockCount_ - nextIndex_;
    }
    [[nodiscard]] std::size_t blockSize() const noexcept
    {
        return stride_;
    }

  private:
    std::unique_ptr<std::byte[]> storage_;
    std::size_t stride_{0};
    std::size_t blockCount_{0};
    std::size_t nextIndex_{0};
};

} // namespace axf::core
