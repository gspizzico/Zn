#include <Async/TaskGraph.h>
#include <Log/LogMacros.h>
#include <CoreAssert.h>
#include <algorithm>

DEFINE_STATIC_LOG_CATEGORY(LogTaskGraph, ELogVerbosity::Log);

namespace Zn
{
TaskGraph::TaskGraph(Name name_)
    : name(name_)
{
}

Name TaskGraph::GetName() const
{
    return name;
}

void TaskGraph::Enqueue(SharedPtr<ITaskGraphNode> task_, std::initializer_list<SharedPtr<ITaskGraphNode>> dependencies_)
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
    const int32 originalIndex = IndexOf(task_); // -1 if not in Graph

    int32 index = std::max(originalIndex, 0); // be sure to always start at 0

    if (dependencies_.size() > 0)
    {
        for (const auto& dependency : dependencies_)
        {
            auto dependencyIndex = IndexOf(dependency);
            check(dependencyIndex >= 0);

            index = std::max(dependencyIndex + 1, index); // Index is always the greater DependencyIndex + 1.
        }
    }

    if (index > originalIndex)
    {
        // #todo (check if we are executing the graph)
        if (originalIndex != -1) // removed task from previous level.
        {
            auto it = At(originalIndex);
            check(it.has_value());

            (*it)->erase(std::find((**it).begin(), (**it).end(), task_));
        }

        if (graph.size() <= index)
        {
            graph.push_back({task_});
        }
        else
        {
            if (auto it = At(index); it != graph.cend())
            {
                (*it)->emplace_back(task_);
            }
        }
    }
}

void TaskGraph::InsertAfter(SharedPtr<ITaskGraphNode> task_, SharedPtr<ITaskGraphNode> insertAfterTask_)
{
    auto it = At(insertAfterTask_);
    check(it != graph.cend());

    ++it;

    graph.insert(it, {task_});
}

void TaskGraph::DumpNode() const
{
    ZN_LOG(LogTaskGraph, ELogVerbosity::Log, "Graph: %s", name.CString());

    sizet level = 0;
    for (const auto& node : graph)
    {
        ZN_LOG(LogTaskGraph, ELogVerbosity::Log, "Level: %d", level++);

        for (const auto& task : node)
        {
            task->DumpNode();
        }
    }
}

int32 TaskGraph::IndexOf(SharedPtr<ITaskGraphNode> task_)
{
    int32 index = -1;

    for (const auto& node : graph)
    {
        index++;
        if (std::count(node.begin(), node.end(), task_) > 0)
            return index;
    }
    return -1;
}

TaskGraph::GraphType::const_iterator TaskGraph::At(SharedPtr<ITaskGraphNode> task_)
{
    return std::find_if(graph.begin(),
                        graph.end(),
                        [task_](const auto& it)
                        {
                            return std::count(it.begin(), it.end(), task_) > 0;
                        });
}

std::optional<TaskGraph::GraphType::iterator> TaskGraph::At(size_t index_)
{
    if (index_ <= graph.size())
    {
        sizet i = 0;

        for (auto it = graph.begin(); it != graph.end(); ++it)
        {
            if (i == index_)
            {
                return it;
            }

            ++i;
        }
    }

    return std::nullopt;
}
} // namespace Zn
