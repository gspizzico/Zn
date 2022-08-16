#include <Znpch.h>
#include "Automation/AutomationTestManager.h"
#include "Core/HAL/Misc.h"

DEFINE_STATIC_LOG_CATEGORY(LogAutomationTestManager, ELogVerbosity::Log)

namespace Zn::Automation
{
	AutomationTestManager& AutomationTestManager::Get()
	{
		static AutomationTestManager s_Instance;
		return s_Instance;
	}

	Vector<Name> AutomationTestManager::GetStartupTestsNames() const
	{
		Vector<Name> Names;
		Names.reserve(m_StartupTests.size());
		for (const auto Test : m_StartupTests)
		{
			Names.emplace_back(Test->GetName());
		}

		return Names;
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

			Test->Reset();

			CurrentTestIndex++;
		}

		ZN_LOG(LogAutomationTestManager, ELogVerbosity::Log, "Finished executing startup tests");
	}

	void AutomationTestManager::EnqueueTests(const Vector<Name>& testNames)
	{
		for (const Name& TestName : testNames)
		{
			if (auto ResultIt = std::find_if(std::begin(m_StartupTests), std::end(m_StartupTests), [TestName](const SharedPtr<AutomationTest>& It) { return It->GetName() == TestName; }); ResultIt != std::end(m_StartupTests))
			{
				m_PendingTests.emplace_back(*ResultIt);
			}
		}
	}

	void AutomationTestManager::Tick(float deltaTime)
	{
		if (m_PendingTests.size() > 0)
		{
			auto& CurrentTest = m_PendingTests[0];

			if (CurrentTest->GetState() != AutomationTest::State::kReadyToExit)
			{
				CurrentTest->Run();
			}
			else
			{
				CurrentTest->Reset();
				m_PendingTests.erase(std::begin(m_PendingTests));
			}
		}
	}
}