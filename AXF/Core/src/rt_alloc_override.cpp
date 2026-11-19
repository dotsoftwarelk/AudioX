// Optional global allocation instrumentation. This translation unit is built
// only into the dedicated audit test target; it is not part of the installed
// AXF SDK. Do not enable it in shipping builds.
#include "axf/core/rt.hpp"

#include <cstdlib>
#include <new>

#if defined(_WIN32)
#include <malloc.h>
#endif

namespace
{
void* auditedAlignedMalloc(std::size_t size, std::size_t alignment)
{
#if defined(_WIN32)
    return _aligned_malloc(size == 0 ? 1 : size, alignment);
#else
    const std::size_t rounded = ((size == 0 ? 1 : size) + alignment - 1) & ~(alignment - 1);
    return std::aligned_alloc(alignment, rounded);
#endif
}

void auditedAlignedFree(void* pointer) noexcept
{
#if defined(_WIN32)
    _aligned_free(pointer);
#else
    std::free(pointer);
#endif
}
} // namespace

void* operator new(std::size_t size)
{
    void* pointer = std::malloc(size == 0 ? 1 : size);
    if (pointer == nullptr)
    {
        throw std::bad_alloc();
    }
    axf::core::rt::noteAllocation(size, false);
    return pointer;
}

void* operator new[](std::size_t size)
{
    return ::operator new(size);
}

void* operator new(std::size_t size, const std::nothrow_t&) noexcept
{
    void* pointer = std::malloc(size == 0 ? 1 : size);
    if (pointer != nullptr)
    {
        axf::core::rt::noteAllocation(size, false);
    }
    return pointer;
}

void* operator new[](std::size_t size, const std::nothrow_t& tag) noexcept
{
    return ::operator new(size, tag);
}

void* operator new(std::size_t size, std::align_val_t alignment)
{
    void* pointer = auditedAlignedMalloc(size, static_cast<std::size_t>(alignment));
    if (pointer == nullptr)
    {
        throw std::bad_alloc();
    }
    axf::core::rt::noteAllocation(size, false);
    return pointer;
}

void* operator new[](std::size_t size, std::align_val_t alignment)
{
    return ::operator new(size, alignment);
}

void* operator new(std::size_t size, std::align_val_t alignment, const std::nothrow_t&) noexcept
{
    void* pointer = auditedAlignedMalloc(size, static_cast<std::size_t>(alignment));
    if (pointer != nullptr)
    {
        axf::core::rt::noteAllocation(size, false);
    }
    return pointer;
}

void* operator new[](std::size_t size, std::align_val_t alignment,
                     const std::nothrow_t& tag) noexcept
{
    return ::operator new(size, alignment, tag);
}

void operator delete(void* pointer) noexcept
{
    if (pointer != nullptr)
    {
        axf::core::rt::noteAllocation(0, true);
        std::free(pointer);
    }
}

void operator delete[](void* pointer) noexcept
{
    ::operator delete(pointer);
}

void operator delete(void* pointer, const std::nothrow_t&) noexcept
{
    ::operator delete(pointer);
}

void operator delete[](void* pointer, const std::nothrow_t&) noexcept
{
    ::operator delete(pointer);
}

void operator delete(void* pointer, std::align_val_t alignment) noexcept
{
    if (pointer != nullptr)
    {
        axf::core::rt::noteAllocation(0, true);
        auditedAlignedFree(pointer);
    }
    (void)alignment;
}

void operator delete[](void* pointer, std::align_val_t alignment) noexcept
{
    ::operator delete(pointer, alignment);
}

void operator delete(void* pointer, std::size_t, std::align_val_t alignment) noexcept
{
    ::operator delete(pointer, alignment);
}

void operator delete[](void* pointer, std::size_t, std::align_val_t alignment) noexcept
{
    ::operator delete(pointer, alignment);
}

void operator delete(void* pointer, std::size_t) noexcept
{
    ::operator delete(pointer);
}

void operator delete[](void* pointer, std::size_t) noexcept
{
    ::operator delete(pointer);
}
