#pragma once
#include "Core/Memory/Allocators/PoolAllocator.h"
#include "Core/Memory/VirtualMemory.h"
#include <array>
#include <set>
#include <list>

namespace Zn
{
	class FixedSizeAllocator
	{
	public:
		struct alignas(8) FreeBlock
		{
			uint32_t m_AllocationToken;
			uint32_t m_NextBlockOffset;
			
			static constexpr uint32_t kValidationToken = 0xfbaf;		//FreeBlockAllocationFlag
		};		

		struct FSAPage
		{
			FSAPage(size_t page_size, size_t allocation_size);

			size_t m_Size;

			size_t m_AllocationSize;

			size_t m_AllocatedBlocks;

			FixedSizeAllocator::FreeBlock* m_NextFreeBlock;

			bool IsFull() const;

			size_t MaxAllocations() const;

			void* Allocate();

			void Free(void* address);
		
		private:

			FixedSizeAllocator::FreeBlock* StartAddress() const;
		};

		static constexpr size_t	kMinAllocationSize = sizeof(uintptr_t);							// Each block stores the address to the next block.

		FixedSizeAllocator(size_t allocationSize, SharedPtr<MemoryPool> memoryPool);

		FixedSizeAllocator(size_t allocationSize, size_t pageSize);

		void* Allocate();

		void Free(void* address);		

	private:		

		void AllocatePage();

		SharedPtr<MemoryPool>	m_MemoryPool;

		size_t					m_AllocationSize;

		FreeBlock*				m_NextFreeBlock;

		// Book keeping data

		std::list<uintptr_t>		m_FreePageList;
		
		std::set<uintptr_t>			m_FullPageList;
	};
}
