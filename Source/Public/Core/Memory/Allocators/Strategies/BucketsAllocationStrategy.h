#pragma once

#include "Core/Memory/VirtualMemory.h"
#include "Core/Memory/Allocators/PageAllocator.h"
#include "Core/Memory/Allocators/FixedSizeAllocator.h"

/*
	Allocation Strategy that uses buckets. Their size is incremental by m_AllocationStep.

	Uses multiple FixedSizeAllocator that share the same reserved memory addresses.
	Each allocator has a unique size of allocation.

	The cost of Allocate & Free is O(1).
*/
namespace Zn
{
	class BucketsAllocationStrategy
	{
	public:

		static constexpr size_t kMinAllocationSize = sizeof(uintptr_t);

		BucketsAllocationStrategy(SharedPtr<PageAllocator> memory, size_t max_allocation_size, size_t allocation_step = sizeof(uintptr_t));

		void* Allocate(size_t size, size_t alignment = sizeof(uintptr_t));

		void Free(void* address);

		size_t GetMaxAllocationSize() const;

		size_t GetWastedMemory() const;		// SLOW!

	private:

		size_t m_AllocationStep;

		SharedPtr<PageAllocator> m_Memory;

		Vector<FixedSizeAllocator> m_Buckets;
	};
}
