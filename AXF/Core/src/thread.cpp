#include "axf/core/thread.hpp"

#include <string>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__linux__)
#include <pthread.h>
#endif

namespace axf::core::thread
{

void setCurrentThreadName(std::string_view name) noexcept
{
#if defined(_WIN32)
    std::wstring wide(name.begin(), name.end());
    if (wide.size() > 255)
    {
        wide.resize(255);
    }
    ::SetThreadDescription(::GetCurrentThread(), wide.c_str());
#elif defined(__linux__)
    std::string narrow(name.substr(0, 15));
    ::pthread_setname_np(::pthread_self(), narrow.c_str());
#else
    (void)name;
#endif
}

std::uint64_t currentThreadId() noexcept
{
#if defined(_WIN32)
    return static_cast<std::uint64_t>(::GetCurrentThreadId());
#elif defined(__linux__)
    return static_cast<std::uint64_t>(::pthread_self());
#else
    return 0;
#endif
}

} // namespace axf::core::thread
