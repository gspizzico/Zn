#pragma once
#include "Core/HAL/BasicTypes.h"
#include "Core/Containers/Set.h"
#include <functional>

namespace Zn
{
	class DirectAllocationStrategy
	{
	public:

		DirectAllocationStrategy(size_t min_allocation_size);

		void* Allocate(size_t size, size_t alignment = sizeof(uintptr_t));

		bool Free(void* address);

	private:

		size_t m_MinAllocationSize;

		UnorderedSet<void*> m_Allocations;
	};
}