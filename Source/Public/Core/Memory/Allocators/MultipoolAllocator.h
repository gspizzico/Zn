#pragma once

#include "Core/Memory/Allocators/PageAllocator.h"
#include "Core/Containers/Vector.h"

namespace Zn
{
	class MultipoolAllocator
	{
	public:

		MultipoolAllocator();

		MultipoolAllocator(size_t pools_num, size_t pool_address_space, size_t min_allocation_size);

		void* Allocate(size_t size, size_t alignment = sizeof(uintptr_t));

		bool Free(void* address);

		size_t GetMaxAllocationSize() const;

	private:

		size_t m_MinAllocationSize;

		Vector<PageAllocator> m_Pools;
	};
}
