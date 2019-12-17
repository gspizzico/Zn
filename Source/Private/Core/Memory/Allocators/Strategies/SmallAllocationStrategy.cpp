#include "Core/Memory/Allocators/Strategies/SmallAllocationStrategy.h"
#include "Core/Log/Log.h"

DEFINE_STATIC_LOG_CATEGORY(LogSmallAllocationStrategy, ELogVerbosity::Log);

namespace Zn
{
	SmallAllocationStrategy::SmallAllocationStrategy(SharedPtr<PageAllocator> memory, size_t max_allocation_size)
		: m_Memory(memory)
		, m_Allocators()
	{
		_ASSERT(max_allocation_size >= kMinAllocationSize);

		const size_t AllocatorsCount = size_t(Memory::Align(max_allocation_size, sizeof(uintptr_t)) / sizeof(uintptr_t));
		m_Allocators.reserve(AllocatorsCount);

		for (size_t Index = 0; Index < AllocatorsCount; ++Index)
		{
			m_Allocators.emplace_back(FixedSizeAllocator(sizeof(uintptr_t) * (Index + 1), m_Memory));
		}
	}

	SmallAllocationStrategy::SmallAllocationStrategy(size_t reserve_memory_size, size_t max_allocation_size)
		: SmallAllocationStrategy(
			  std::make_shared<PageAllocator>(PageAllocator(VirtualMemory::AlignToPageSize(reserve_memory_size), VirtualMemory::GetPageSize()))
			, max_allocation_size)
	{
	}

	void* SmallAllocationStrategy::Allocate(size_t size, size_t alignment)
	{
		const size_t InternalAlignment = Memory::Align(alignment, sizeof(uintptr_t));	// Always alignt to at least 8bytes

		const size_t AllocationSize = Memory::Align(size, InternalAlignment);
		
		_ASSERT(AllocationSize <= GetMaxAllocationSize());								// Ensure that we can allocate this size.

		size_t AllocatorIndex = (AllocationSize / sizeof(uintptr_t)) - 1;

		_ASSERT(AllocatorIndex >= 0 && AllocatorIndex < m_Allocators.size());

		return m_Allocators[AllocatorIndex].Allocate();
	}

	void SmallAllocationStrategy::Free(void* address)
	{
		_ASSERT(m_Memory->Range().Contains(address));

		auto PageAddress = FixedSizeAllocator::FSAPage::GetPageFromAnyAddress(address, m_Memory->Range().Begin(), m_Memory->PageSize());

		const size_t& AllocationSize = PageAddress->m_AllocationSize;

		size_t AllocatorIndex = (AllocationSize / sizeof(uintptr_t)) - 1;

		_ASSERT(AllocatorIndex >= 0 && AllocatorIndex < m_Allocators.size());

		m_Allocators[AllocatorIndex].Free(address);
	}

	size_t SmallAllocationStrategy::GetMaxAllocationSize() const
	{
		return m_Allocators.size() * sizeof(uintptr_t);
	}

	size_t SmallAllocationStrategy::GetWastedMemory() const
	{
		size_t SumAllUsedMemory = 0;

		for(size_t Index = 0; Index < m_Allocators.size(); ++Index)
		{
			const auto& Allocator = m_Allocators[Index];

			const auto& FreePageList = Allocator.GetFreePageList();

			const size_t CommittedFreePagesTotalSize = FreePageList.size() * VirtualMemory::GetPageSize();

			if (CommittedFreePagesTotalSize > 0)
			{
				size_t SumUsedMemory = 0;

				for (const auto& Page : Allocator.GetFreePageList())
				{
					auto pPage = reinterpret_cast<FixedSizeAllocator::FSAPage*>(Page);
				
					SumUsedMemory += pPage->GetAllocatedMemory();
				}
			
				ZN_LOG(LogSmallAllocationStrategy, ELogVerbosity::Verbose, 
					"Allocator %p \t AllocationSize %i \t Usage: %.2f", 
					&Allocator, (Index + 1) * sizeof(uintptr_t), float(SumUsedMemory) / float(CommittedFreePagesTotalSize));

				SumAllUsedMemory += SumUsedMemory;
			}
		}

		return m_Memory->GetUsedMemory() - SumAllUsedMemory;
	}
}