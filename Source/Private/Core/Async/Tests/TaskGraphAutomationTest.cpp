#include <Znpch.h>
#include "Automation/AutomationTest.h"
#include "Automation/AutomationTestManager.h"
#include "Core/Async/TaskGraph.h"

DEFINE_STATIC_LOG_CATEGORY(LogAutomationTest_TaskGraph, ELogVerbosity::Log)

namespace Zn::Automation
{

class AutomationTaskClass : public ITaskGraphNode
{
  public:
    AutomationTaskClass(Name name)
        : m_Name(name)
    {
    }

    Name m_Name;

    virtual Name GetName() const override
    {
        return m_Name;
    }

    virtual void Execute() override {};

    virtual void DumpNode() const override
    {
        ZN_LOG(LogAutomationTest_TaskGraph, ELogVerbosity::Log, "Task Name: %s", m_Name.CString());
    }
};

class TaskGraphAutomationTest : public AutomationTest
{
  public:
    TaskGraphAutomationTest() = default;

    template<typename T> SharedPtr<T> New(Name name)
    {
        return std::make_shared<T>(name);
    }

    virtual void Execute() override
    {
        auto Graph = New<TaskGraph>("Main");

        auto A = New<AutomationTaskClass>("A");
        auto B = New<AutomationTaskClass>("B");
        auto C = New<AutomationTaskClass>("C");
        auto D = New<AutomationTaskClass>("D");
        auto E = New<AutomationTaskClass>("E");
        auto F = New<AutomationTaskClass>("F");
        auto G = New<AutomationTaskClass>("G");
        auto H = New<AutomationTaskClass>("H");
        auto I = New<AutomationTaskClass>("I");

        Graph->Enqueue(A, {});
        Graph->Enqueue(B, {A});
        Graph->Enqueue(C, {B});
        Graph->Enqueue(D, {B});
        Graph->Enqueue(E, {B, C});
        Graph->Enqueue(F, {B});
        Graph->Enqueue(G, {C, D});
        Graph->Enqueue(H, {G, F});

        auto SubA = New<TaskGraph>("SubA");

        Graph->Enqueue(SubA, {A});

        Graph->InsertAfter(I, H);

        Graph->DumpNode();

        m_State = State::kComplete;
    }
};
} // namespace Zn::Automation

DEFINE_AUTOMATION_STARTUP_TEST(TaskGraphAutomationTest, Zn::Automation::TaskGraphAutomationTest);