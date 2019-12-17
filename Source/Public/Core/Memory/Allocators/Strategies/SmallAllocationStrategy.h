#pragma once

#include "Core/Memory/VirtualMemory.h"
#include "Core/Memory/Allocators/PageAllocator.h"
#include "Core/Memory/Allocators/FixedSizeAllocator.h"

/*
	Allocation Strategy for small allocations.

	Uses multiple FixedSizeAllocator that share the same reserved memory addresses.
	Each allocator has a unique size of allocation.

	The cost of Allocate & Free is O(1).
*/
namespace Zn
{
	class SmallAllocationStrategy
	{
	public:

		static constexpr size_t kMinAllocationSize = sizeof(uintptr_t);

		SmallAllocationStrategy(SharedPtr<PageAllocator> memory, size_t max_allocation_size);

		SmallAllocationStrategy(size_t reserve_memory_size, size_t max_allocation_size);

		void* Allocate(size_t size, size_t alignment = sizeof(uintptr_t));

		void Free(void* address);

		size_t GetMaxAllocationSize() const;

		size_t GetWastedMemory() const;		// SLOW!

	private:

		SharedPtr<PageAllocator> m_Memory;

		Vector<FixedSizeAllocator> m_Allocators;
	};
}
