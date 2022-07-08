#include "Automation/AutomationTest.h"
#include "Automation/AutomationTestManager.h"
#include "Core/Log/LogMacros.h"
#include "Core/Async/ThreadManager.h"
#include "Core/Async/ThreadedJob.h"
#include "Core/Async/Thread.h"

DEFINE_STATIC_LOG_CATEGORY(LogAutomationTest_Thread, ELogVerbosity::Log)

#define THREAD_START_PROCEDURE(_THREAD_START_PROCEDURE) _THREAD_START_PROCEDURE


namespace Zn::Automation
{
	class ThreadAutomationJob : public ThreadedJob
	{
	public:
		
		virtual void DoWork() override
		{
			ZN_LOG(LogAutomationTest_Thread, ELogVerbosity::Log, "Log: Tid:%d", PlatformThreads::GetCurrentThreadId());
		}
	};

	class ThreadAutomationTest : public AutomationTest
	{
		Vector<ThreadAutomationJob> m_Jobs;

		static constexpr int32 kJobs = 100;

	public:

		ThreadAutomationTest() = default;

		virtual void Prepare() override
		{
			for (int32 i = 0; i < kJobs; i++)
			{
				m_Jobs.emplace_back(ThreadAutomationJob());
			}
		}

		virtual void Execute() override
		{
			auto& Manager = ThreadManager::Get();

			Vector<UniquePtr<Thread>> Handles;

			for (auto& Job : m_Jobs)
			{
				Handles.emplace_back(std::unique_ptr<Thread>(std::move(Thread::New("ThreadAutomationJob", &Job))));
			}

			for (auto& Handle : Handles)
			{
				Handle->WaitUntilCompletion();
			}

			m_State = State::kComplete;
		}
	};
}

DEFINE_AUTOMATION_STARTUP_TEST(ThreadAutomationTest, Zn::Automation::ThreadAutomationTest);