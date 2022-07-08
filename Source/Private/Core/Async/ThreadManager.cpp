#include "Core/Async/ThreadManager.h"
#include "Core/Log/LogMacros.h"
#include "Core/HAL/Misc.h"
#include "Core/Async/Thread.h"

DEFINE_STATIC_LOG_CATEGORY(LogThreads, ELogVerbosity::Log);

namespace Zn
{
	bool IsMainThread()
	{
		return false;//return ThreadManager::Get().GetCurrentThreadInfo()->GetThreadId() == s_MainThreadId;
	}

	void Sleep(uint32 ms)
	{
		PlatformThreads::Sleep(ms);
	}

	thread_local Thread* ThreadManager::s_CurrentThread = nullptr;

	uint32 ThreadManager::s_MainThreadId = 0;

	ThreadManager& ThreadManager::Get()
	{
		static ThreadManager s_Instance;
		return s_Instance;
	}

	void ThreadManager::Startup()
	{
		s_MainThreadId = PlatformThreads::GetCurrentThreadId();
	}

//	void ThreadManager::AddThread(Thread* new_thread)
//	{
//		SharedPtr<Thread> NewThread(new_thread);
//
//		TScopedLock<CriticalSection> Lock(&cs_m_Threads);
//
//		m_Threads.emplace_back(std::move(NewThread));
//	}
//
//	void ThreadManager::RemoveThread(Thread* thread)
//	{
//		uint32 ThreadId = thread->GetId();
//
//		TScopedLock<CriticalSection> Lock(&cs_m_Threads);
//
//		for (auto Index = 0; Index < m_Threads.size(); ++Index)
//		{
//			auto It = std::begin(m_Threads) + Index;
//			auto Thread = *It;
//			if (Thread->GetId() == ThreadId) // We have found our thread.
//			{
////				_ASSERT(Thread->IsAlive() && Thread->IsCurrentThread());
//				m_Threads.erase(It);
//				break;
//			}
//		}
//	}

	//void ThreadManager::RemoveThread(NativeThreadId id)
	//{
	//	TScopedLock<CriticalSection> Lock(&cs_m_Threads);
	//	
	//	for (auto Index = 0; Index < m_ThreadsInfo.size(); ++Index)
	//	{
	//		auto It = std::begin(m_ThreadsInfo) + Index;
	//		auto Thread = *It;
	//		if (Thread->GetThreadId() == id) // We have found our thread.
	//		{
	//			_ASSERT(Thread->IsAlive() && Thread->IsCurrentThread());
	//			m_ThreadsInfo.erase(It);
	//			break;
	//		}
	//	}
	//}

	//SharedPtr<ThreadInfo> ThreadManager::GetThreadInfo(NativeThreadId id) const
	//{
	//	for (auto It = std::begin(m_ThreadsInfo); It != std::end(m_ThreadsInfo); ++It)
	//	{
	//		auto Thread = *It;
	//		if (Thread->GetThreadId() == id) // We have found our thread.
	//		{
	//			return Thread;
	//		}
	//	}
	//	return nullptr;
	//}

	//ThreadInfo* ThreadManager::GetCurrentThreadInfo()
	//{
	//	if (s_CurrentThread) return s_CurrentThread;

	//	auto ThreadInfoPtr = ThreadManager::Get().GetThreadInfo(PlatformThreads::GetCurrentThreadId());

	//	s_CurrentThread = ThreadInfoPtr.get();

	//	return s_CurrentThread;
	//}
	
}