#pragma once

#include "Core/HAL/BasicTypes.h"
#include "Core/HAL/PlatformTypes.h"
#include "Core/Name.h"
#include "Core/Time/Time.h"
#include "Core/Async/ScopedLock.h"

namespace Zn
{
	bool IsMainThread();

	void Sleep(uint32 ms);

	//typedef void (*ThreadFunctionPtr)(void*);

	//class ThreadInfo
	//{
	//public:

	//	ThreadInfo(NamedThread thread);

	//	ThreadInfo(NamedThread thread, ThreadFunctionPtr function, ThreadArgsPtr args);

	//	~ThreadInfo();

	//	bool IsAlive() const { return PlatformThreads::IsThreadAlive(m_ThreadHandle); }

	//	bool IsAttached() const { return m_Attached; }

	//	bool IsCurrentThread() const { return GetThreadId() == PlatformThreads::GetCurrentThreadId(); }

	//	void Wait();

	//	bool Wait(uint32 ms);

	//	NativeThreadId GetThreadId() const { return m_Id; }

	//	NamedThread GetName() const { return m_ThreadName; }

	//	void Start();

	//	void Abort();

	//private:

	//	NamedThread m_ThreadName;

	//	NativeThreadId m_Id;

	//	NativeThreadHandle m_ThreadHandle = NULL;

	//	ThreadFunctionPtr m_Function = NULL;

	//	ThreadArgsPtr m_Arguments = NULL;

	//	bool m_Attached = false;

	//	uint64 m_StartTime = ULLONG_MAX;

	//	void Run();

	//	THREAD_FUNCTION_SIGNATURE(RunThread, arg)
	//	{
	//		ThreadInfo* Thread = reinterpret_cast<ThreadInfo*>(arg);
	//		Thread->Run();
	//		THREAD_FUNCTION_RETURN;
	//	}
	//};

	class ThreadManager
	{
	public:

		static ThreadManager& Get();

		void Startup();

		void AddThread(Thread* new_thread);
		
		void RemoveThread(Thread* thread);

		/*void RemoveThread(NativeThreadId id);

		const Thread* GetThreadInfo(NativeThreadId id) const;

		static Thread* GetCurrentThreadInfo();*/

	private:

		//		static constexpr uint16 kInvalidThreadId = std::numeric_limits<uint16>::max();

		//SharedPtr<ThreadInfo> m_MainThread;

		thread_local static Thread* s_CurrentThread;

		//Vector<SharedPtr<ThreadInfo>> m_ThreadsInfo;

		Vector<SharedPtr<Thread>> m_Threads;

		CriticalSection cs_m_Threads;

		static uint32 s_MainThreadId;
	};

	/*template<NamedThread ThreadType>
	SharedPtr<ThreadInfo> ThreadManager::StartThread(ThreadFunctionPtr function, ThreadArgsPtr arguments)
	{
		static_assert(ThreadType != NamedThread::MainThread);

		SharedPtr<ThreadInfo> Handle = std::make_shared<ThreadInfo>(ThreadType, function, arguments);

		{
			TScopedLock<CriticalSection> Lock(&cs_m_Threads);
			m_ThreadsInfo.emplace_back(Handle);
		}

		Handle->Start();

		return Handle;
	}*/
}
