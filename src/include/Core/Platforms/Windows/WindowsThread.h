#pragma once
#include "WindowsCommon.h"
#include "Core/Types.h"
#include "Core/Async/Thread.h"

namespace Zn
{
class WindowsThread : public Thread
{
  public:
    WindowsThread();

    virtual void WaitUntilCompletion() override;

    virtual bool Wait(uint32 ms_) override;

    virtual ~WindowsThread() override;

    virtual bool HasValidHandle() const override;

  protected:
    virtual bool Start(ThreadedJob* job_) override;

  private:
    HANDLE handle;

    static DWORD WINAPI RunThread(LPVOID thread_);
};
} // namespace Zn
