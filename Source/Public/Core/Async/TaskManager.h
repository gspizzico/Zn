#pragma once

#include "Core/HAL/BasicTypes.h"
#include "Core/HAL/Guid.h"
#include "Core/Containers/Vector.h"
#include "Core/Async/ITaskGraphNode.h"
#include "Core/Name.h"

namespace Zn
{
	using UniquePrerequisitesId = Guid;

	// Express the dependencies of one task.
	struct TaskDependency : public SharedFromThis<TaskDependency>
	{
#if ZN_DEBUG
		Name m_Name;									// Task Name. Can be stripped in Release.
#endif
		int32 m_RemainingTasks = 1;						// Numbers of tasks to be executed before this task is complete. 1 means that the last action is this task execution.
		Vector<SharedPtr<TaskDependency>> m_Successors;	// Successors of this task.

		void Visit() const;
	};

	// Defines an handle owned by a task. Used to uniquely identify an instance of a task.
	class TaskHandle : public SharedFromThis<TaskHandle>
	{
	public:

		enum
		{
			kInvalidHandle
		};

		friend class TaskManager;

		TaskHandle(enum kInvalidHandle) {};
		
		TaskHandle(): m_TaskId(Guid::Generate())
			, m_Dependency(std::make_shared<TaskDependency>())
		{
		}

		// Add a predecessor to this task.
		template<typename TTaskType, typename ...Args>
		SharedPtr<TaskHandle> Requires(Args... args);

		// Add predecessors to this task
		void Requires(Vector<SharedPtr<TaskHandle>>&& tasks);

		// Add a successor to this task.
		template<typename TTaskType, typename ...Args>
		SharedPtr<TaskHandle> Then(Args... args);

		// Add successors to this task.
		void Then(Vector<SharedPtr<TaskHandle>>&& tasks);

		SharedPtr<TaskHandle> AsShared() { return shared_from_this(); }

	private:

		Guid m_TaskId = Guid::kNone;
		SharedPtr<TaskDependency> m_Dependency;
	};

	class TaskManager
	{
	public:

		static TaskManager& Get();

		// Creates a simple task of the supplied type.
		template<typename TTaskType, typename ...Args>
		SharedPtr<TaskHandle> CreateTask(Args... args);

		void Dispatch();

		void DumpState(); // DEBUG

	private:

		Vector<SharedPtr<ITaskGraphNode>> m_Tasks;		
	};


	template<typename TTaskType, typename ...Args>
	inline SharedPtr<TaskHandle> TaskManager::CreateTask(Args ...args)
	{
		static_assert(std::is_base_of_v<ITaskGraphNode, TTaskType>);

		SharedPtr<ITaskGraphNode> Task = std::make_shared<TTaskType>(std::forward<Args>(args)...);
		SharedPtr<TaskHandle> Handle = std::make_shared<TaskHandle>();
#if ZN_DEBUG
		Handle->m_Dependency->m_Name = Task->GetName();
#endif
		Task->m_Handle = Handle;
		m_Tasks.emplace_back(Task);
		return Handle;
	}

	template<typename TTaskType, typename ...Args>
	inline SharedPtr<TaskHandle> TaskHandle::Requires(Args ...args)
	{
		auto& Manager = TaskManager::Get();
		auto Handle = Manager.CreateTask<TTaskType>(std::forward<Args>(args)...);
		Handle->Then({ AsShared() });
		return Handle;
	}

	template<typename TTaskType, typename ...Args>
	SharedPtr<TaskHandle> TaskHandle::Then(Args ...args)
	{
		auto& Manager = TaskManager::Get();
		auto Handle = Manager.CreateTask<TTaskType>(std::forward<Args>(args)...);
		Then({ Handle });
		return Handle;
	}
}
