#pragma once

#include <Core/CoreTypes.h>

namespace Zn
{
class ThreadedJob;

class Thread
{
  public:
    friend class ThreadManager;

    enum class Type
    {
        MainThread,
        HighPriorityWorkerThread,
        WorkerThread
    };

    static Thread* New(String name_, ThreadedJob* job_);

    uint32 GetId() const
    {
        return threadId;
    }

    // Thread::Type GetType() const { return m_Type; }

    bool IsCurrentThread() const;

    virtual bool HasValidHandle() const = 0;

    virtual void WaitUntilCompletion() = 0;

    virtual bool Wait(uint32 ms_) = 0;

    virtual ~Thread();

  protected:
    virtual bool Start(ThreadedJob* job_) = 0;

    uint32 Main();

    uint32 threadId = 0;

    ThreadedJob* job {nullptr};

  private:
    String name;

    // Thread::Type m_Type;
};
} // namespace Zn
