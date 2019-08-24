#pragma once

#include "Core/Memory/VirtualMemory.h"

namespace Zn
{
	class HeapAllocator
	{
	public:

		HeapAllocator();

		HeapAllocator(size_t memory_region_size, size_t page_size);
		
		HeapAllocator(VirtualMemoryHeap&& heap, size_t page_size);
		
		HeapAllocator(HeapAllocator&&);
		
		virtual ~HeapAllocator();

		HeapAllocator(const HeapAllocator&) = delete;
		
		HeapAllocator& operator=(const HeapAllocator&) = delete;

		void* AllocatePage();

		bool Free(void* address);

	private:

		VirtualMemoryHeap		m_MemoryHeap;

		size_t					m_PageSize;

		size_t					m_RegionIndex;

		void*					m_Address;

		static constexpr size_t		kDefaultPageSize	= 1 << 16;			// 64k

		static constexpr size_t		kDefaultRegionSize	= 32 * (1 << 20);	// 32Mb
	};
}
