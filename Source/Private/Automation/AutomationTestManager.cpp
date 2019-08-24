#include "Automation/AutomationTestManager.h"

#include "Core/HAL/Misc.h"

namespace Zn::Automation
{
	AutomationTestManager& AutomationTestManager::Get()
	{
		static AutomationTestManager s_Instance;
		return s_Instance;
	}

	void AutomationTestManager::ExecuteStartupTests()
	{
		size_t CurrentTestIndex = 0;

		while (CurrentTestIndex < m_StartupTests.size())
		{
			auto&& Test = m_StartupTests[CurrentTestIndex];

			Test->Prepare();

			do
			{
			} while (Test->Run() != AutomationTest::State::kReadyToExit);

			CurrentTestIndex++;
		}
	}
}