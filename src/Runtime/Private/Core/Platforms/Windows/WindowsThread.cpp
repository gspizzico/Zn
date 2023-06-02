#include <Windows/WindowsThread.h>
#include <Core/CoreAssert.h>

namespace Zn
{
WindowsThread::WindowsThread()
    : handle(NULL)
{
    // #TODO Create Sync point to avoid  that RunThread is called before Initialize has been completed.

    handle = ::CreateThread(
        NULL, 0, WindowsThread::RunThread, this, CREATE_SUSPENDED /*| STACK_SIZE_PARAM_IS_A_RESERVATION*/, (DWORD*) (&threadId));
}

void WindowsThread::WaitUntilCompletion()
{
    check(handle);
    ::WaitForSingleObject(handle, INFINITE);
}

bool WindowsThread::Wait(uint32 ms_)
{
    check(handle);
    return ::WaitForSingleObject(handle, ms_) == WAIT_OBJECT_0;
}

WindowsThread::~WindowsThread()
{
    if (handle)
    {
        ::TerminateThread(handle, 0);
    }
}

bool WindowsThread::HasValidHandle() const
{
    return handle != NULL;
}

bool WindowsThread::Start(ThreadedJob* job_)
{
    if (handle != NULL)
    {
        job = job_;
        ::ResumeThread(handle);

        return true;
    }

    return false;
}

DWORD WINAPI WindowsThread::RunThread(LPVOID thread_)
{
    check(thread_ != nullptr);

    WindowsThread* winThread = reinterpret_cast<WindowsThread*>(thread_);

    return winThread->Main();
}
} // namespace Zn
