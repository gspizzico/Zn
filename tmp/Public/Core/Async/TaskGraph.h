#pragma once
#include "Core/Async/ITaskGraphNode.h"
#include "Core/HAL/BasicTypes.h"
#include <list>
#include <optional>

/*
    A TaskGraph represents a sequence of nodes to be executed.
    A node is an instance of an ITaskGraphNode. The graph itself can be a node to be executed by another graph.
*/
namespace Zn
{
class TaskGraph : public ITaskGraphNode
{
  public:
    TaskGraph(Name name);

    virtual void Execute() override {}; // #todo

    virtual Name GetName() const override;

    virtual void DumpNode() const override;

    // Enqueues a task onto the graph, or to update its dependencies.
    // The update is always incremental. Ex. A -> B -> C, then let's say C depends only on A, C will not be pushed back but will stay in place.
    // @param task - the task to be pushed
    // @param dependencies - the list of tasks from which this task is dependent. These must be already pushed into the graph.
    void Enqueue(SharedPtr<ITaskGraphNode> task, std::initializer_list<SharedPtr<ITaskGraphNode>> dependencies);

    // Inserts a task onto the graph. It is used to "push forward" the tasks following @insert_after_task
    void InsertAfter(SharedPtr<ITaskGraphNode> task, SharedPtr<ITaskGraphNode> insert_after_task);

  private:
    using GraphType = std::list<Vector<SharedPtr<ITaskGraphNode>>>;

    GraphType m_Graph;

    Name m_Name;

    int32_t IndexOf(SharedPtr<ITaskGraphNode> task);

    GraphType::const_iterator At(SharedPtr<ITaskGraphNode> task);

    std::optional<GraphType::iterator> At(size_t index);
};
} // namespace Zn