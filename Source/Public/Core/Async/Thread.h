#pragma once
#include "Core/HAL/BasicTypes.h"

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

		static Thread* New(String name, ThreadedJob* job);

		uint32 GetId() const { return m_ThreadId; }

		//Thread::Type GetType() const { return m_Type; }

		bool IsCurrentThread() const;

		virtual bool HasValidHandle() const = 0;

		virtual void WaitUntilCompletion() = 0;

		virtual bool Wait(uint32 ms) = 0;

		virtual ~Thread();

	protected:

		virtual bool Start(ThreadedJob* job) = 0;

		uint32 Main();

		uint32 m_ThreadId = 0;

		ThreadedJob* m_Job{ nullptr };
	
	private:

		String m_Name;

		//Thread::Type m_Type;
	};
}
