#pragma once

#include "Core/Async/ThreadedJob.h"
#include <functional>

namespace Zn
{
	class CallableJob : public ThreadedJob
	{
		CallableJob(std::function<void()>&& callable)
			: m_Callable(std::move(callable)) {}

		virtual void DoWork() override
		{
			m_Callable();
		}

		virtual void Finalize() override
		{
			m_Callable = nullptr;
		}

	private:

		std::function<void()> m_Callable;
	};
}
