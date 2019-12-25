#include "Core/Memory/Allocators/Strategies/DirectAllocationStrategy.h"
#include "Core/Memory/VirtualMemory.h"

namespace Zn
{
	DirectAllocationStrategy::DirectAllocationStrategy(size_t min_allocation_size)
		: m_MinAllocationSize(min_allocation_size)
	{
	}

	void* DirectAllocationStrategy::Allocate(size_t size, size_t alignment)
	{
		_ASSERT(alignment >= sizeof(uintptr_t));

		size_t AllocationSize = size += sizeof(size_t);

		AllocationSize = VirtualMemory::AlignToPageSize(size);

		_ASSERT(AllocationSize < Memory::GetMemoryStatus().m_AvailPhys);

		auto Address = VirtualMemory::Allocate(AllocationSize);

		m_Allocations.emplace(reinterpret_cast<uintptr_t>(Address));

		MemoryDebug::MarkUninitialized(Address, Memory::AddOffset(Address, AllocationSize));

		new (Address) size_t(AllocationSize);

		return Memory::AddOffset(Address, sizeof(size_t));
	}

	void DirectAllocationStrategy::Free(void* address)
	{
		auto AllocationAddress = Memory::SubOffset(address, sizeof(size_t));

		auto Removed = m_Allocations.erase(reinterpret_cast<uintptr_t>(AllocationAddress));

		_ASSERT(Removed != 0);

		//size_t AllocationSize = *reinterpret_cast<size_t*>(AllocationAddress);

		VirtualMemory::Release(AllocationAddress);
	}
}
