#include <Corepch.h>
#include "Windows/WindowsThreads.h"
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
NativeThreadHandle WindowsThreads::OpenThread(NativeThreadId threadId_)
{
    NativeThreadHandle handle = ::OpenThread(THREAD_ALL_ACCESS, FALSE, static_cast<DWORD>(threadId_));
    if (handle == NULL)
    {
        ZN_LOG(LogWindowsThreads,
               ELogVerbosity::Error,
               "Unable to open thread with id %d. Error: %d",
               threadId_,
               PlatformMisc::GetLastError());
    }
    return handle;
}

NativeThreadHandle WindowsThreads::CreateThread(NativeThreadId&         outThreadId_,
                                                NativeThreadFunctionPtr function_,
                                                ThreadArgsPtr           args_,
                                                bool                    startSuspended_ /*= false*/)
{
    return NativeThreadHandle(::CreateThread(NULL, 0, function_, args_, startSuspended_ ? CREATE_SUSPENDED : 0, &outThreadId_));
}

void WindowsThreads::SetThreadPriority(NativeThreadHandle handle_, ThreadPriority priority_)
{
    ::SetThreadPriority(handle_, ToNativePriority(priority_));
}

void WindowsThreads::CloseThread(NativeThreadHandle handle_)
{
    ::CloseHandle(handle_);
}

void WindowsThreads::KillThread(NativeThreadHandle handle_)
{
    ::TerminateThread(handle_, 0);
}

NativeThreadId WindowsThreads::GetCurrentThreadId()
{
    return NativeThreadId(::GetCurrentThreadId());
}

bool WindowsThreads::IsThreadAlive(NativeThreadHandle handle_)
{
    return handle_ != NULL && ::WaitForSingleObject(handle_, 0) == WAIT_TIMEOUT;
}

bool WindowsThreads::WaitThread(NativeThreadHandle handle_, uint32 ms_)
{
    check(handle_ != nullptr);
    return ::WaitForSingleObject(handle_, ms_) == WAIT_OBJECT_0;
}

void WindowsThreads::WaitThread(NativeThreadHandle handle_)
{
    check(handle_ != nullptr);
    ::WaitForSingleObject(handle_, INFINITE);
}

void WindowsThreads::Sleep(uint32 ms_)
{
    ::Sleep(ms_);
}

int32 WindowsThreads::ToNativePriority(ThreadPriority priority_)
{
    switch (priority_)
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
