#pragma once
#include "Core/HAL/BasicTypes.h"

#include "Automation/AutomationTest.h"

namespace Zn::Automation
{
	class AutomationTestManager
	{
	public:

		static AutomationTestManager& Get();

		template<typename TTestType, typename... Args>
		static void RegisterStartupTest(Name name, Args... arguments);

		Vector<Name> GetStartupTestsNames() const;

		void ExecuteStartupTests();

		void EnqueueTests(const Vector<Name>& testNames);

		void Tick(float deltaTime);

		bool IsRunningTests() const
		{
			return m_PendingTests.size() > 0;
		}

	private:

		AutomationTestManager() = default;							// Private constructor for singleton.

		Vector<SharedPtr<AutomationTest>>	m_StartupTests;			// These tests are executed only on engine startup.

		Vector<SharedPtr<AutomationTest>>	m_PendingTests;			// Queue of pending tests to execute. Test at index 0 is current test.

		//#todo Make a test_type or several vectors of tests to be executed in different moments.
	};

	template<typename TTestType, typename ...Args>
	inline void AutomationTestManager::RegisterStartupTest(Name name, Args ...arguments)
	{
		auto& Manager = AutomationTestManager::Get();
		auto Test = std::make_shared<TTestType>(TTestType(std::forward<Args>(arguments)...));
		Test->m_Name = name;
		Manager.m_StartupTests.emplace_back(Test);
	}

	template<typename TTestType>
	struct AutoStartupTest
	{
	public:

		template<typename ...Args>
		AutoStartupTest(Name name, Args ...arguments)
		{
			AutomationTestManager::RegisterStartupTest<TTestType>(name, std::forward<Args>(arguments)...);
		}
	};
}

#define DEFINE_AUTOMATION_STARTUP_TEST(Name, Type, ...) namespace { Zn::Automation::AutoStartupTest<Type> Name##TestInstance{#Name, __VA_ARGS__};}