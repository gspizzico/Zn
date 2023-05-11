#pragma once
#include "Core/HAL/BasicTypes.h"
#include "WindowsCommon.h"

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
    static NativeThreadHandle OpenThread(NativeThreadId thread_id);

    // Creates a new thread of execution.
    static NativeThreadHandle CreateThread(NativeThreadId& out_thread_id, NativeThreadFunctionPtr function, ThreadArgsPtr args, bool start_suspended = false);

    // Sets the thread priority.
    static void SetThreadPriority(NativeThreadHandle handle, ThreadPriority priority);

    // Close a thread by its handle.
    static void CloseThread(NativeThreadHandle handle);

    // Terminates a thread. USE ONLY IN EXTREME CIRCUMSTANCES AS IT DOES NOT CLEAN UP.
    static void KillThread(NativeThreadHandle handle);

    // Retrieves this thread id.
    static NativeThreadId GetCurrentThreadId();

    // Checks if thread is alive and running.
    static bool IsThreadAlive(NativeThreadHandle handle);

    // Waits for a thread for @param ms milliseconds before timing out. Returns true if thread returned before timing out.
    static bool WaitThread(NativeThreadHandle handle, uint32 ms);

    // Waits for a thread indefinitely until it finishes execution.
    static void WaitThread(NativeThreadHandle handle);

    // Suspends the execution of the current thread until the time-out interval elapses.
    static void Sleep(uint32 ms);

  private:
    static int32 ToNativePriority(ThreadPriority priority);
};
} // namespace Zn
