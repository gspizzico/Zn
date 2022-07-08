#pragma once
#include <functional>
#include "Core/HAL/BasicTypes.h"
#include "Core/Name.h"
#include <mutex>

namespace Zn::Automation
{
	class AutomationTest
	{
	public:

		enum class Result : uint8_t
		{
			kOk			= 1,
			kCannotRun	= 1 << 1,
			kFailed		= 1 << 2,
			kCritical	= 1 << 3,
			kNone		= 1 << 4
		};

		enum class State
		{
			kUninitialized,
			kReady,
			kRunning,
			kComplete,
			kReadyToExit
		};

		virtual ~AutomationTest() = default;

		AutomationTest() = default;

		AutomationTest(const AutomationTest&);

		State Run();

		void Terminate(bool bForce);

		virtual void Prepare() {};

		virtual void Execute() {};

		virtual void Cleanup(bool bForce) {};

		Name GetName() const { return m_Name; };

		virtual String GetErrorMessage() const { return String(); };

		State GetState() const { return m_State; }

		virtual bool ShouldQuitWhenCriticalError() const { return false; }

		virtual bool HasAsyncOperationsPending() const { return false; }

		virtual void Sync() { };

	protected:

		friend class AutomationTestManager;

		Name m_Name;

		std::mutex	mtx_State;

		State		m_State			= State::kUninitialized;

		Result		m_Result		= Result::kNone;
	};
}

#define ZN_TEST_VERIFY(Expression, ResultType)\
{\
	using namespace Zn::Automation;\
	static_assert(ResultType == AutomationTest::Result::kCritical ||ResultType == AutomationTest::Result::kFailed);\
	std::scoped_lock Lock(this->mtx_State); \
	if(this->m_State == AutomationTest::State::kReadyToExit) return;\
	if (!(Expression))\
	{\
		this->m_Result = ResultType; \
		if (ResultType == AutomationTest::Result::kCritical)\
		return; \
	}\
	else\
	{\
	m_Result = AutomationTest::Result::kOk; \
	}\
}
