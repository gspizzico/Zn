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

		size_t GetPageSize() const { return m_PageSize; }

		size_t GetAllocatedMemory() const;

		bool IsValidAddress(void* address) const { return m_MemoryHeap.IsValidAddress(address); }

		bool IsAllocated(void* address) const;

	private:

		VirtualMemoryHeap		m_MemoryHeap;

		size_t					m_PageSize;

		size_t					m_RegionIndex;

		void*					m_NextPageAddress;

		static constexpr size_t		kDefaultPageSize	= 1 << 16;			// 64k

		static constexpr size_t		kDefaultRegionSize	= 32 * (1 << 20);	// 32Mb
	};
}
