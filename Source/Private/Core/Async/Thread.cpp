#include "Core/Async/Thread.h"
#include "Core/HAL/PlatformTypes.h"
#include "Core/Log/LogMacros.h"
#include "Core/Async/ThreadedJob.h"

DEFINE_STATIC_LOG_CATEGORY(LogThread, ELogVerbosity::Log)

namespace Zn
{
	Thread::~Thread()
	{
		//ThreadManager::Get().RemoveThread(this);
	}

	Thread* Thread::New(String name, ThreadedJob* job)
	{
		if (job == nullptr) return nullptr;

		Thread* NewThread = PlatformThreads::CreateNewThread();

		_ASSERT(NewThread != nullptr);

		NewThread->m_Name = name.length() > 0 ? name : "Unnamed Thread";

		if (!NewThread->Start(job))
		{
			ZN_LOG(LogThread, ELogVerbosity::Warning, "Failed to create a thread.");

			delete NewThread;
			NewThread = nullptr;
		}

		return NewThread;
	}

	inline bool Thread::IsCurrentThread() const { return GetId() == PlatformThreads::GetCurrentThreadId(); }

	uint32 Thread::Main()
	{
		_ASSERT(m_Job != nullptr);

		m_Job->Prepare();
		m_Job->DoWork();
		m_Job->Finalize();

		return 0;
	}
}