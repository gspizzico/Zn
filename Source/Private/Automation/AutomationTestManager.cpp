#include "Automation/AutomationTestManager.h"
#include "Core/Log/LogMacros.h"
#include "Core/HAL/Misc.h"

DEFINE_STATIC_LOG_CATEGORY(LogAutomationTestManager, ELogVerbosity::Log)

namespace Zn::Automation
{
	AutomationTestManager& AutomationTestManager::Get()
	{
		static AutomationTestManager s_Instance;
		return s_Instance;
	}

	void AutomationTestManager::ExecuteStartupTests()
	{
		ZN_LOG(LogAutomationTestManager, ELogVerbosity::Log, "Executing startup tests. %i tests are registered.", m_StartupTests.size());

		size_t CurrentTestIndex = 0;

		while (CurrentTestIndex < m_StartupTests.size())
		{
			auto&& Test = m_StartupTests[CurrentTestIndex];

			ZN_LOG(LogAutomationTestManager, ELogVerbosity::Log, "Executing test n.%i:\t%s", CurrentTestIndex, Test->GetName().CString());

			do
			{
			} while (Test->Run() != AutomationTest::State::kReadyToExit);

			CurrentTestIndex++;
		}

		ZN_LOG(LogAutomationTestManager, ELogVerbosity::Log, "Finished executing startup tests");
	}
}