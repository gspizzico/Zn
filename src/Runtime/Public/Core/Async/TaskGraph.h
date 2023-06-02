#pragma once
#include <Core/CoreTypes.h>
#include <Core/Misc/Name.h>
#include <Core/Async/ITaskGraphNode.h>
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
    // The update is always incremental. Ex. A -> B -> C, then let's say C depends only on A, C will not be pushed back but will stay in
    // place.
    // @param task - the task to be pushed
    // @param dependencies - the list of tasks from which this task is dependent. These must be already pushed into the graph.
    void Enqueue(SharedPtr<ITaskGraphNode> task_, std::initializer_list<SharedPtr<ITaskGraphNode>> dependencies_);

    // Inserts a task onto the graph. It is used to "push forward" the tasks following @insert_after_task
    void InsertAfter(SharedPtr<ITaskGraphNode> task_, SharedPtr<ITaskGraphNode> insertAfterTask_);

  private:
    using GraphType = std::list<Vector<SharedPtr<ITaskGraphNode>>>;

    GraphType graph;

    Name name;

    int32 IndexOf(SharedPtr<ITaskGraphNode> task_);

    GraphType::const_iterator At(SharedPtr<ITaskGraphNode> task_);

    std::optional<GraphType::iterator> At(size_t index_);
};
} // namespace Zn
