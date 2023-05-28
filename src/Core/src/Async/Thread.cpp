#include <Async/Thread.h>
#include <Async/ThreadedJob.h>
#include <Log/LogMacros.h>
#include <CorePlatform.h>
#include <CoreAssert.h>

DEFINE_STATIC_LOG_CATEGORY(LogThread, ELogVerbosity::Log)

namespace Zn
{
Thread::~Thread()
{
    // ThreadManager::Get().RemoveThread(this);
}

Thread* Thread::New(String name_, ThreadedJob* job_)
{
    if (job_ == nullptr)
        return nullptr;

    Thread* newThread = PlatformThreads::CreateNewThread();

    check(newThread != nullptr);

    newThread->name = name_.length() > 0 ? name_ : "Unnamed Thread";

    if (!newThread->Start(job_))
    {
        ZN_LOG(LogThread, ELogVerbosity::Warning, "Failed to create a thread.");

        delete newThread;
        newThread = nullptr;
    }

    return newThread;
}

inline bool Thread::IsCurrentThread() const
{
    return GetId() == PlatformThreads::GetCurrentThreadId();
}

uint32 Thread::Main()
{
    check(job != nullptr);

    job->Prepare();
    job->DoWork();
    job->Finalize();

    return 0;
}
} // namespace Zn