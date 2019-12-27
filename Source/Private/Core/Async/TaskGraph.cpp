#include "Core/Async/TaskGraph.h"
#include <algorithm>
#include "Core/Log/LogMacros.h"

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
		const int32_t OriginalIndex = IndexOf(task);	// -1 if not in Graph

		int32_t Index = std::max(OriginalIndex, 0);		// be sure to always start at 0

		if (dependencies.size() > 0)
		{
			for (const auto& Dependency : dependencies)
			{
				auto DependencyIndex = IndexOf(Dependency);
				_ASSERT(DependencyIndex >= 0);

				Index = std::max(DependencyIndex + 1, Index);	// Index is always the greater DependencyIndex + 1.
			}
		}

		if (Index > OriginalIndex)
		{
			// #todo (check if we are executing the graph)
			if (OriginalIndex != -1)			// removed task from previous level.
			{
				auto It = At(OriginalIndex);
				_ASSERT(It.has_value());

				(*It)->erase(std::find((**It).begin(), (**It).end(), task));
			}

			if (m_Graph.size() <= Index)
			{
				m_Graph.push_back({ task });
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
		_ASSERT(It != m_Graph.cend());
		
		++It;

		m_Graph.insert(It, { task });
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
			if(std::count(Elem.begin(), Elem.end(), task) > 0)
				return Index;
		}
		return -1;
	}

	TaskGraph::GraphType::const_iterator TaskGraph::At(SharedPtr<ITaskGraphNode> task)
	{
		return std::find_if(m_Graph.begin(), m_Graph.end(), [task](const auto& It) { return std::count(It.begin(), It.end(), task) > 0; });
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
}
