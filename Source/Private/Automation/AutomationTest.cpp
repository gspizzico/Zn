#include "Automation/AutomationTest.h"
#include "Core/Log/LogMacros.h"
#include "Core/HAL/Misc.h"
#include "Core/Time/Time.h"

DEFINE_STATIC_LOG_CATEGORY(LogAutomationTest, ELogVerbosity::Log);

namespace Zn::Automation
{
	using namespace Zn;

	AutomationTest::AutomationTest(const AutomationTest& other)
		: m_State(other.m_State)
		, m_Result(other.m_Result)
	{
		_ASSERT(m_State == State::kUninitialized);
	}

	AutomationTest::State AutomationTest::Run()
	{
		switch (m_State)
		{
		case State::kUninitialized:
		{
			Prepare();
			m_State = State::kReady;
			break;
		}
		case State::kReady:
		{
			m_State = State::kRunning;
			m_StartTime = Time::Seconds();

			Execute();
			break;
		}
		case State::kRunning:
		{
			if (!HasAsyncOperationsPending()) m_State = State::kComplete;
			break;
		}
		case State::kComplete:
		{
			Terminate(false);
			break;
		}
		default:
			break;
		}

		return m_State;
	}

	void AutomationTest::Terminate(bool bForce)
	{
		std::scoped_lock Lock(mtx_State);

		if (m_State == State::kRunning)
		{
			if (HasAsyncOperationsPending())
			{
				Sync();
			}
		}

		m_State = State::kReadyToExit;

		const float Duration = static_cast<float>(Time::Seconds() - m_StartTime);

		const auto TestNameString = GetName().CString();

		const auto ErrorMessage = GetErrorMessage();

		const auto ErrorMessageCString = ErrorMessage.c_str();

		ZN_LOG(LogAutomationTest, ELogVerbosity::Log, "%s was completed in %.3f seconds.", TestNameString, Duration);

		switch (m_Result)
		{
		case Result::kCannotRun:
			ZN_LOG(LogAutomationTest, ELogVerbosity::Warning, "%s could not be run for the following reason: \n\t\t %s", TestNameString, ErrorMessageCString);
			break;
		case Result::kFailed:
			ZN_LOG(LogAutomationTest, ELogVerbosity::Error, "%s has failed for the following reason: \n\t\t %s", TestNameString, ErrorMessageCString);
			break;
		case Result::kOk:
			ZN_LOG(LogAutomationTest, ELogVerbosity::Log, "%s is successful.", TestNameString, ErrorMessageCString);
			break;
		case Result::kCritical:
		{
			ZN_LOG(LogAutomationTest, ELogVerbosity::Error, "%s has critically failed. Application will be closed. Reason: \n\t\t %s", TestNameString, ErrorMessageCString);

			if (ShouldQuitWhenCriticalError())
			{
				Misc::Exit(true);
			}
		}
		}
	}

	void AutomationTest::Reset()
	{
		Cleanup();

		m_State = State::kUninitialized;
		m_Result = Result::kNone;
		m_StartTime = 0.0;
	}
}