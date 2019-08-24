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
		static void RegisterStartupTest(Args... arguments);

		void ExecuteStartupTests();

	private:

		AutomationTestManager() = default;							// Private constructor for singleton.

		Vector<SharedPtr<AutomationTest>>	m_StartupTests;			// These tests are executed only on engine startup.

		//#todo Make a test_type or several vectors of tests to be executed in different moments.
	};

	template<typename TTestType, typename ...Args>
	inline void AutomationTestManager::RegisterStartupTest(Args ...arguments)
	{
		auto& Manager = AutomationTestManager::Get();
		Manager.m_StartupTests.emplace_back(std::make_shared<TTestType>(TTestType(std::forward<Args>(arguments)...)));
	}
	
	template<typename TTestType>
	struct AutoStartupTest
	{
	public:

		template<typename ...Args>
		AutoStartupTest(Args ...arguments)
		{
			AutomationTestManager::RegisterStartupTest<TTestType>(std::forward<Args>(arguments)...);
		}
	};
}

#define DEFINE_AUTOMATION_STARTUP_TEST(Name, Type, ...) namespace { Zn::Automation::AutoStartupTest<Type> Name##TestInstance{__VA_ARGS__};}