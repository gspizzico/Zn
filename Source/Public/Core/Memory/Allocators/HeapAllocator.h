#pragma once

#include "Core/Memory/VirtualMemory.h"
#include "Core/Containers/Map.h"

namespace Zn
{
	class HeapAllocator
	{
	public:

		static constexpr size_t		kDefaultPageSize	= 1 << 16;				// 64k

		static constexpr size_t		kDefaultRegionSize	= 32 * (1 << 20);		// 32Mb

		HeapAllocator();

		HeapAllocator(size_t memory_region_size, size_t page_size);
		
		HeapAllocator(VirtualMemoryHeap&& heap, size_t page_size);
		
		HeapAllocator(HeapAllocator&&) noexcept;
		
		virtual ~HeapAllocator();

		HeapAllocator(const HeapAllocator&) = delete;
		
		HeapAllocator& operator=(const HeapAllocator&) = delete;

		void* AllocatePage();

		bool Free(void* address);

		size_t GetPageSize() const { return m_PageSize; }

		size_t GetReservedMemory() const;

		bool IsValidAddress(void* address) const { return m_MemoryHeap.IsValidAddress(address); }

		bool IsAllocated(void* address) const;

		bool Free(MemoryRange pages);

	private:

		UnorderedMap<size_t, Vector<void*>>			m_FreePageList;

		VirtualMemoryHeap		m_MemoryHeap;

		size_t					m_PageSize;

		size_t					m_RegionIndex;

		void*					m_NextPageAddress;
	};
}
