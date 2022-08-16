#include <Znpch.h>
#include "Automation/AutomationTest.h"
#include "Automation/AutomationTestManager.h"

DEFINE_STATIC_LOG_CATEGORY(LogAutomationTestExample, ELogVerbosity::Log)

namespace Zn::Automation
{
	class AutomationTestExample : public AutomationTest
	{
	public:

		AutomationTestExample()
			: m_NumberOfIterations(1)
		{}

		AutomationTestExample(size_t number_of_iterations)
			: m_NumberOfIterations(number_of_iterations)
		{}

		virtual void Prepare() override
		{
			ZN_LOG(LogAutomationTestExample, ELogVerbosity::Verbose, "AutomationTestExample... prepare.");
		}

		virtual void Execute() override
		{
			for (size_t i = 0; i < m_NumberOfIterations; ++i)
			{
				size_t j = i + 1;

				static bool s_ForceFail = false;
				static constexpr Result s_FailResult = Result::kCritical;

				const bool bResult = s_ForceFail ? j == !j : j == i + 1;

				ZN_TEST_VERIFY(bResult, s_FailResult);
			}
		}

		virtual void Cleanup() override
		{
			ZN_LOG(LogAutomationTestExample, ELogVerbosity::Verbose, "AutomationTestExample... cleanup.");
		}

		virtual bool ShouldQuitWhenCriticalError() const override
		{
			static bool s_ForceQuit = false;
			return s_ForceQuit;
		}

	private:

		size_t m_NumberOfIterations;
	};
}

DEFINE_AUTOMATION_STARTUP_TEST(AutomationTestExample, Zn::Automation::AutomationTestExample, 5)