#include "axf/core/thread.hpp"

#include <string>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__linux__)
#include <pthread.h>
#include <sys/resource.h>
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

Result<void> setCurrentThreadPriority(ThreadPriority priority) noexcept
{
#if defined(_WIN32)
    int value = THREAD_PRIORITY_NORMAL;
    switch (priority)
    {
    case ThreadPriority::Low:
        value = THREAD_PRIORITY_BELOW_NORMAL;
        break;
    case ThreadPriority::Normal:
        value = THREAD_PRIORITY_NORMAL;
        break;
    case ThreadPriority::High:
        value = THREAD_PRIORITY_ABOVE_NORMAL;
        break;
    }
    return ::SetThreadPriority(::GetCurrentThread(), value) != FALSE
               ? success()
               : makeError(ErrorCode::ThreadError, "SetThreadPriority failed");
#elif defined(__linux__)
    int niceValue = 0;
    switch (priority)
    {
    case ThreadPriority::Low:
        niceValue = 5;
        break;
    case ThreadPriority::Normal:
        niceValue = 0;
        break;
    case ThreadPriority::High:
        niceValue = -5;
        break;
    }
    return ::setpriority(PRIO_PROCESS, 0, niceValue) == 0
               ? success()
               : makeError(ErrorCode::ThreadError, "setpriority failed (check permissions)");
#else
    (void)priority;
    return makeError(ErrorCode::Unsupported, "thread priority is not implemented on this OS");
#endif
}

const char* threadPriorityName(ThreadPriority priority) noexcept
{
    switch (priority)
    {
    case ThreadPriority::Low:
        return "low";
    case ThreadPriority::Normal:
        return "normal";
    case ThreadPriority::High:
        return "high";
    }
    return "unknown";
}

} // namespace axf::core::thread
