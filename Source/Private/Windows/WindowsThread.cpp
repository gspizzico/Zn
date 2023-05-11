#include <Znpch.h>
#include "Windows/WindowsThread.h"

namespace Zn
{
WindowsThread::WindowsThread()
    : m_Handle(NULL)
{
    // #TODO Create Sync point to avoid  that RunThread is called before Initialize has been completed.

    m_Handle = ::CreateThread(NULL, 0, WindowsThread::RunThread, this, CREATE_SUSPENDED /*| STACK_SIZE_PARAM_IS_A_RESERVATION*/, (DWORD*) (&m_ThreadId));
}

void WindowsThread::WaitUntilCompletion()
{
    _ASSERT(m_Handle);
    ::WaitForSingleObject(m_Handle, INFINITE);
}

bool WindowsThread::Wait(uint32 ms)
{
    _ASSERT(m_Handle);
    return ::WaitForSingleObject(m_Handle, ms) == WAIT_OBJECT_0;
}

WindowsThread::~WindowsThread()
{
    if (m_Handle)
    {
        ::TerminateThread(m_Handle, 0);
    }
}

bool WindowsThread::HasValidHandle() const
{
    return m_Handle != NULL;
}

bool WindowsThread::Start(ThreadedJob* job)
{
    if (m_Handle != NULL)
    {
        m_Job = job;
        ::ResumeThread(m_Handle);

        return true;
    }

    return false;
}

DWORD WINAPI WindowsThread::RunThread(LPVOID p_thread)
{
    _ASSERT(p_thread != nullptr);

    WindowsThread* pThread = reinterpret_cast<WindowsThread*>(p_thread);

    return pThread->Main();
}
} // namespace Zn