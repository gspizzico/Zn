#include <Corepch.h>
#include "Windows/WindowsThreads.h"
#include "HAL/PlatformTypes.h"
#include "Windows/WindowsThread.h"

DEFINE_STATIC_LOG_CATEGORY(LogWindowsThreads, ELogVerbosity::Log);

namespace Zn
{
Thread* WindowsThreads::CreateNewThread()
{
    WindowsThread* thread = new WindowsThread();
    if (!thread->HasValidHandle())
    {
        ZN_LOG(LogWindowsThreads, ELogVerbosity::Error, "Failed to create a thread. Error: %d", PlatformMisc::GetLastError());

        delete thread;

        return nullptr;
    }

    return thread;
}
NativeThreadHandle WindowsThreads::OpenThread(NativeThreadId thread_id)
{
    NativeThreadHandle Handle = ::OpenThread(THREAD_ALL_ACCESS, FALSE, static_cast<DWORD>(thread_id));
    if (Handle == NULL)
    {
        ZN_LOG(LogWindowsThreads,
               ELogVerbosity::Error,
               "Unable to open thread with id %d. Error: %d",
               thread_id,
               PlatformMisc::GetLastError());
    }
    return Handle;
}

NativeThreadHandle WindowsThreads::CreateThread(NativeThreadId&         out_thread_id,
                                                NativeThreadFunctionPtr function,
                                                ThreadArgsPtr           args,
                                                bool                    start_suspended /*= false*/)
{
    DWORD              ThreadId;
    NativeThreadHandle Handle =
        NativeThreadHandle(::CreateThread(NULL, 0, function, args, start_suspended ? CREATE_SUSPENDED : 0, &ThreadId));
    out_thread_id = ThreadId;
    return Handle;
}

void WindowsThreads::SetThreadPriority(NativeThreadHandle handle, ThreadPriority priority)
{
    ::SetThreadPriority(handle, ToNativePriority(priority));
}

void WindowsThreads::CloseThread(NativeThreadHandle handle)
{
    ::CloseHandle(handle);
}

void WindowsThreads::KillThread(NativeThreadHandle handle)
{
    ::TerminateThread(handle, 0);
}

NativeThreadId WindowsThreads::GetCurrentThreadId()
{
    return NativeThreadId(::GetCurrentThreadId());
}

bool WindowsThreads::IsThreadAlive(NativeThreadHandle handle)
{
    return handle != NULL && ::WaitForSingleObject(handle, 0) == WAIT_TIMEOUT;
}

bool WindowsThreads::WaitThread(NativeThreadHandle handle, uint32 ms)
{
    check(handle != nullptr);
    return ::WaitForSingleObject(handle, ms) == WAIT_OBJECT_0;
}

void WindowsThreads::WaitThread(NativeThreadHandle handle)
{
    check(handle != nullptr);
    ::WaitForSingleObject(handle, INFINITE);
}

void WindowsThreads::Sleep(uint32 ms)
{
    ::Sleep(ms);
}

int32 WindowsThreads::ToNativePriority(ThreadPriority priority)
{
    switch (priority)
    {
    case ThreadPriority::Idle:
        return THREAD_PRIORITY_IDLE;
    case ThreadPriority::Lowest:
        return THREAD_PRIORITY_LOWEST;
    case ThreadPriority::Low:
        return THREAD_PRIORITY_BELOW_NORMAL;
    case ThreadPriority::High:
        return THREAD_PRIORITY_ABOVE_NORMAL;
    case ThreadPriority::Highest:
        return THREAD_PRIORITY_HIGHEST;
    case ThreadPriority::TimeCritical:
        return THREAD_PRIORITY_TIME_CRITICAL;
    case ThreadPriority::Normal:
    default:
        return THREAD_PRIORITY_NORMAL;
    }
}
} // namespace Zn
