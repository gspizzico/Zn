#pragma once

#include <Core/CoreTypes.h>
#include "Windows/WindowsCommon.h"

#ifndef THREAD_FUNCTION_SIGNATURE
    #define THREAD_FUNCTION_SIGNATURE(RunThread, ArgName) static DWORD WINAPI RunThread(LPVOID ArgName)
#endif

#ifndef THREAD_FUNCTION_RETURN
    #define THREAD_FUNCTION_RETURN return 0
#endif

namespace Zn
{
typedef HANDLE NativeThreadHandle;

typedef DWORD NativeThreadId;

typedef LPTHREAD_START_ROUTINE NativeThreadFunctionPtr;

typedef __drv_aliasesMem LPVOID ThreadArgsPtr;

class Thread;

class WindowsThreads
{
  public:
    static Thread* CreateNewThread();

    // Opens an existing thread object.
    static NativeThreadHandle OpenThread(NativeThreadId threadId_);

    // Creates a new thread of execution.
    static NativeThreadHandle CreateThread(NativeThreadId&         outThreadId_,
                                           NativeThreadFunctionPtr function_,
                                           ThreadArgsPtr           args_,
                                           bool                    startSuspended_ = false);

    // Sets the thread priority.
    static void SetThreadPriority(NativeThreadHandle handle_, ThreadPriority priority_);

    // Close a thread by its handle.
    static void CloseThread(NativeThreadHandle handle_);

    // Terminates a thread. USE ONLY IN EXTREME CIRCUMSTANCES AS IT DOES NOT CLEAN UP.
    static void KillThread(NativeThreadHandle handle_);

    // Retrieves this thread id.
    static NativeThreadId GetCurrentThreadId();

    // Checks if thread is alive and running.
    static bool IsThreadAlive(NativeThreadHandle handle_);

    // Waits for a thread for @param ms milliseconds before timing out. Returns true if thread returned before timing out.
    static bool WaitThread(NativeThreadHandle handle_, uint32 ms_);

    // Waits for a thread indefinitely until it finishes execution.
    static void WaitThread(NativeThreadHandle handle_);

    // Suspends the execution of the current thread until the time-out interval elapses.
    static void Sleep(uint32 ms_);

  private:
    static int32 ToNativePriority(ThreadPriority priority_);
};
} // namespace Zn
