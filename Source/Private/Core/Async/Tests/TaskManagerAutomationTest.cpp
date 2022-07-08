#include "Automation/AutomationTest.h"
#include "Automation/AutomationTestManager.h"
#include "Core/Log/LogMacros.h"
#include "Core/Async/TaskManager.h"

DEFINE_STATIC_LOG_CATEGORY(LogAutomationTest_TaskManager, ELogVerbosity::Log)

namespace Zn::Automation
{

	class AutomationTaskClass2 : public ITaskGraphNode
	{
	public:

		AutomationTaskClass2(Name name)
			: m_Name(name)
		{
		}

		Name m_Name;

		virtual Name GetName() const override { return m_Name; }

		virtual void Execute() override 
		{
			DumpNode();
		};

		virtual void DumpNode() const override
		{
			ZN_LOG(LogAutomationTest_TaskManager, ELogVerbosity::Log, "Task Name: %s", m_Name.CString());
		}
	};

	class TaskManagerAutomationTest : public AutomationTest
	{
	public:

		TaskManagerAutomationTest() = default;

		virtual void Execute() override
		{
			auto TManager = TaskManager::Get();
			
			{
				auto A = TManager.CreateTask<AutomationTaskClass2>("A");
				auto B = TManager.CreateTask<AutomationTaskClass2>("B");
				auto C = TManager.CreateTask<AutomationTaskClass2>("C");
				auto D = TManager.CreateTask<AutomationTaskClass2>("D");
				auto E = TManager.CreateTask<AutomationTaskClass2>("E");
				auto F = TManager.CreateTask<AutomationTaskClass2>("F");
				auto G = TManager.CreateTask<AutomationTaskClass2>("G");
				auto H = TManager.CreateTask<AutomationTaskClass2>("H");

				A->Then({ B });
				B->Then({ C, D, E, F });
				C->Then({ E, G });
				D->Then({ G });
				F->Then({ H });
				G->Then({ H });

				TManager.DumpState();
				TManager.Dispatch();
			}
			
			{
				auto F = TManager.CreateTask<AutomationTaskClass2>("F1");
				auto D = TManager.CreateTask<AutomationTaskClass2>("D1");
				auto C = TManager.CreateTask<AutomationTaskClass2>("C1");
				auto E = TManager.CreateTask<AutomationTaskClass2>("E1");
				auto G = TManager.CreateTask<AutomationTaskClass2>("G1");
				auto A = TManager.CreateTask<AutomationTaskClass2>("A1");
				auto H = TManager.CreateTask<AutomationTaskClass2>("H1");
				auto B = TManager.CreateTask<AutomationTaskClass2>("B1");

				B->Requires({ A });
				C->Requires({ B });
				D->Requires({ B });
				E->Requires({ B, C });
				F->Requires({ B });
				G->Requires({ C, D });
				H->Requires({ F, G });

				TManager.DumpState();
				TManager.Dispatch();
			}

			m_State = State::kComplete;
		}
	};
}

DEFINE_AUTOMATION_STARTUP_TEST(TaskManagerAutomationTest, Zn::Automation::TaskManagerAutomationTest);