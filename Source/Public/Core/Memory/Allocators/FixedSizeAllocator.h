#pragma once
#include "Core/Memory/Allocators/PoolAllocator.h"
#include "Core/Memory/VirtualMemory.h"
#include <array>
#include <map>
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

		struct Page
		{
			Page();

			Page(MemoryRange pageRange, size_t allocationSize);
			
			Page(Page&& other) noexcept;
		
			VirtualMemoryPage						m_Page;

			size_t									m_AllocationSize;

			FixedSizeAllocator::FreeBlock*			m_NextFreeBlock;

			void* Allocate();

			void Free(void* address);
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

		std::map<uintptr_t, Page>	m_Pages;

		std::list<uintptr_t>		m_FreePageList;
	};
}
