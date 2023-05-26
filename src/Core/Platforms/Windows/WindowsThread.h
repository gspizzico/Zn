#pragma once
#include "WindowsCommon.h"
#include "Types.h"
#include "Async/Thread.h"

namespace Zn
{
class WindowsThread : public Thread
{
  public:
    WindowsThread();

    virtual void WaitUntilCompletion() override;

    virtual bool Wait(uint32 ms) override;

    virtual ~WindowsThread() override;

    virtual bool HasValidHandle() const override;

  protected:
    virtual bool Start(ThreadedJob* job) override;

  private:
    HANDLE m_Handle;

    static DWORD WINAPI RunThread(LPVOID p_thread);
};
} // namespace Zn
