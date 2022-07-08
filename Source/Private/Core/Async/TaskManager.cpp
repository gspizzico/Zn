#include "Core/Async/TaskManager.h"
#include "Core/Log/LogMacros.h"
#include <algorithm>

DEFINE_STATIC_LOG_CATEGORY(LogTaskManager, ELogVerbosity::Log)

namespace Zn
{
	TaskManager& TaskManager::Get()
	{
		static TaskManager Instance;
		return Instance;
	}

	void TaskManager::Dispatch()
	{
		//todo
		while (m_Tasks.size() > 0)
		{
			for (size_t i = 0; i < m_Tasks.size(); i++)
			{
				auto Task = m_Tasks[i];
				if (Task->m_Handle->m_Dependency->m_RemainingTasks == 1)
				{
					Task->m_Handle->m_Dependency->m_RemainingTasks--;
					Task->Execute();
					
					m_Tasks.erase(std::begin(m_Tasks) + i);

					for (auto Successor : Task->m_Handle->m_Dependency->m_Successors)
					{
						Successor->m_RemainingTasks--;
					}

					break;
				}
			}
		}
	}

	void TaskManager::DumpState()
	{
		for (auto Task : m_Tasks)
		{
			ZN_LOG(LogTaskManager, ELogVerbosity::Verbose, "Visiting task %s", Task->GetName().CString());

			Task->m_Handle->m_Dependency->Visit();
		}
	}
	
	void TaskHandle::Requires(Vector<SharedPtr<TaskHandle>>&& tasks)
	{
		for (auto Task : tasks)
		{
			Task->Then({ AsShared() });
		}
	}

	void TaskHandle::Then(Vector<SharedPtr<TaskHandle>>&& tasks)
	{
		for (auto Task : tasks)
		{
			Task->m_Dependency->m_RemainingTasks++;
			m_Dependency->m_Successors.emplace_back(Task->m_Dependency);
		}
	}

	void TaskDependency::Visit() const
	{
		ZN_LOG(LogTaskManager, ELogVerbosity::Verbose, "%s is waiting for %d actions.", m_Name.CString(), m_RemainingTasks);

		for (auto Successor : m_Successors)
		{
			Successor->Visit();
		}
	}
}
