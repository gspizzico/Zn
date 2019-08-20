#pragma once

#include "Core/Memory/Memory.h"
#include "Core/Log/Log.h"
#include <vector>

DECLARE_LOG_CATEGORY(LogMemory);

namespace Zn
{
	struct VirtualMemoryInformation;

    class VirtualMemory
    {
    public:

		enum class State
		{
			kReserved	= 0,	// Indicates reserved pages where a range of the process's virtual address space is reserved without any physical storage being allocated. 
			kCommitted	= 1,	// Indicates committed pages for which physical storage has been allocated, either in memory or in the paging file on disk.
			kFree		= 2		// Indicates free pages not accessible to the calling process and available to be allocated.
		};

        static void* Reserve(size_t size);

        static void* Allocate(size_t size);

        static bool Release(void* address);

        static bool Commit(void* address, size_t size);

        static bool Decommit(void* address, size_t size);

        static size_t GetPageSize();

        static size_t AlignToPageSize(size_t size);

		static VirtualMemoryInformation GetMemoryInformation(void* address, size_t size);
    };

	struct VirtualMemoryInformation
	{
		MemoryRange m_Range;

		VirtualMemory::State m_State;		// Note: State::kFree -> m_Range is undefined.
	};

	struct VirtualMemoryRegion
	{
	public:

		VirtualMemoryRegion()		= default;

		VirtualMemoryRegion(size_t capacity);

		VirtualMemoryRegion(VirtualMemoryRegion&& other) noexcept;

		~VirtualMemoryRegion();

		VirtualMemoryRegion(const VirtualMemoryRegion&) = delete;

		VirtualMemoryRegion& operator=(const VirtualMemoryRegion&) = delete;
		
		void* operator*() const { return m_Address; }

		operator bool() const { return m_Address != nullptr; }

		size_t Size() const		{ return m_Range.Size(); }

		const MemoryRange& Range() const { return m_Range; }

	private:

		void* m_Address		= nullptr;

		MemoryRange m_Range;
	};	

	struct VirtualMemoryHeap
	{
	public:

		VirtualMemoryHeap()			= default;

		VirtualMemoryHeap(size_t region_size);

		VirtualMemoryHeap(VirtualMemoryHeap&& other) noexcept;

		VirtualMemoryHeap(const VirtualMemoryHeap&) = delete;

		VirtualMemoryHeap& operator=(const VirtualMemoryHeap&) = delete;

		const auto& Regions() const { return m_Regions; }

		bool  IsValidAddress(void* address) const;

		void* AllocateRegion();

		bool FreeRegion(size_t region_index);
		
	private:

		size_t m_RegionSize			= 0;
		
		std::vector<SharedPtr<VirtualMemoryRegion>> m_Regions;

		//std::vector<size_t> m_FreeRegions;

	};
}
