#include <Core/Corepch.h>
#include "Core/Async/TaskGraph.h"
#include <algorithm>

DEFINE_STATIC_LOG_CATEGORY(LogTaskGraph, ELogVerbosity::Log);

namespace Zn
{
TaskGraph::TaskGraph(Name name)
    : m_Name(name)
{
}

Name TaskGraph::GetName() const
{
    return m_Name;
}

void TaskGraph::Enqueue(SharedPtr<ITaskGraphNode> task, std::initializer_list<SharedPtr<ITaskGraphNode>> dependencies)
{
    // Task
    // Manager
    // Scheduler
    // Worker Thread
    // Dependency Manager

    /*
        USE CASE:
            1)	Create a list of tasks to be executed, OUTSIDE the graph with explicit dependencies.
                Ex:
                    Task* A = new Task();
                    Task* B = new Task();
                    Task* C = new Task();

                    TaskGraph* G = new TaskGraph();
                    G->Enqueue(A, {});
                    G->Enqueue(B, {A});
                    G->Enqueue(C, {B});

            2)	Create a list of tasks to be executed, INSIDE the graph (each task might spawn a new task)
                Ex:
                    Task::Execute()
                    {
                        // Do stuff...
                        float Value = 0;
                        Task* A = new Task(&Value);
                        Enqueue(A).Wait(); // this might be a problem
                        return;
                    }

                    Task::Execute()
                    {
                        Task* A = new Task();
                        ContinuationTask* B = new ContinuationTask(MyData);

                        Enqueue(A);
                        Enqueue(B, {A});
                        return;
                    }

            TaskHandle::TaskHandle(Task* p, int64 unique_id)
            {
                TaskPtr = p;
                RefCount = 1;
                UniqueDependencyId = unique_id;
            };

            TaskHandle* A = TaskManager::CreateTask<T>(args...)
                        {
                            T* pT = new T(args...);
                            SharedPtr<TaskHandle> Th{pT, -1};
                            Vt.Add(Th);
                            return Th;
                        };

            TaskHandle* B = TaskManager::CreateTask<T2>(args...);

            TaskGraph::Link(B, {A});

            TaskGraph::Link(TaskHandle* Th1, Vector<TaskHandle*> Vt)
            {
                Th1.RefCount += Vt.size();

                for(TaskHandle* Th : Vt)
                {
                    if(Th.UniqueDependencyId == -1)
                    {
                        Th.UniqueDependencyId = NewDependency();
                    }

                    Dependencies.Find(Th.UniqueDependencyId)->Add(Th1);
                }
            };

            TaskManager::Dispatch();

            // 1) TaskManager::CreateTask<T>(args...).Then<T>(args...);
            // 2)	auto H = TaskManager::CreateTask<T>(args);
                    H.Then<T>(args...);
                    H.Then<T>(args...);

                    H.Dispatch();
            // 3)	auto R = TaskManager::CreateNamedGraph("Render");
                    auto L = TaskManager::CreateTask<T>(args...);

                    a) TaskGraph::AddPrerequisite(L, R);
                    b) L.Then("Render"); // Does not require to be dispatched when linked to named graph.




    */
    const int32_t OriginalIndex = IndexOf(task); // -1 if not in Graph

    int32_t Index = std::max(OriginalIndex, 0); // be sure to always start at 0

    if (dependencies.size() > 0)
    {
        for (const auto& Dependency : dependencies)
        {
            auto DependencyIndex = IndexOf(Dependency);
            check(DependencyIndex >= 0);

            Index = std::max(DependencyIndex + 1, Index); // Index is always the greater DependencyIndex + 1.
        }
    }

    if (Index > OriginalIndex)
    {
        // #todo (check if we are executing the graph)
        if (OriginalIndex != -1) // removed task from previous level.
        {
            auto It = At(OriginalIndex);
            check(It.has_value());

            (*It)->erase(std::find((**It).begin(), (**It).end(), task));
        }

        if (m_Graph.size() <= Index)
        {
            m_Graph.push_back({task});
        }
        else
        {
            if (auto It = At(Index); It != m_Graph.cend())
            {
                (*It)->emplace_back(task);
            }
        }
    }
}

void TaskGraph::InsertAfter(SharedPtr<ITaskGraphNode> task, SharedPtr<ITaskGraphNode> insert_after_task)
{
    auto It = At(insert_after_task);
    check(It != m_Graph.cend());

    ++It;

    m_Graph.insert(It, {task});
}

void TaskGraph::DumpNode() const
{
    ZN_LOG(LogTaskGraph, ELogVerbosity::Log, "Graph: %s", m_Name.CString());

    size_t Level = 0;
    for (const auto& Node : m_Graph)
    {
        ZN_LOG(LogTaskGraph, ELogVerbosity::Log, "Level: %d", Level++);

        for (const auto& Task : Node)
        {
            Task->DumpNode();
        }
    }
}

int32_t TaskGraph::IndexOf(SharedPtr<ITaskGraphNode> task)
{
    int32_t Index = -1;

    for (const auto& Elem : m_Graph)
    {
        Index++;
        if (std::count(Elem.begin(), Elem.end(), task) > 0)
            return Index;
    }
    return -1;
}

TaskGraph::GraphType::const_iterator TaskGraph::At(SharedPtr<ITaskGraphNode> task)
{
    return std::find_if(m_Graph.begin(),
                        m_Graph.end(),
                        [task](const auto& It)
                        {
                            return std::count(It.begin(), It.end(), task) > 0;
                        });
}

std::optional<TaskGraph::GraphType::iterator> TaskGraph::At(size_t index)
{
    if (index <= m_Graph.size())
    {
        size_t i = 0;

        for (auto It = m_Graph.begin(); It != m_Graph.end(); ++It)
        {
            if (i == index)
            {
                return It;
            }

            ++i;
        }
    }

    return std::nullopt;
}
} // namespace Zn
