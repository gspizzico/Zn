#pragma once
#include "Core/Memory/VirtualMemory.h"
#include "Core/Containers/Set.h"

namespace Zn
{
	class PageAllocator
	{
	public:

		static constexpr uint64_t kFreePagePattern = 0xfb;

		PageAllocator(size_t pool_size, size_t block_size);

		PageAllocator(size_t block_size);

		PageAllocator(SharedPtr<VirtualMemoryRegion> region, size_t block_size);

		size_t GetUsedMemory() const { return m_AllocatedPages * PageSize(); }

		float GetMemoryUtilization() const { return (float)GetUsedMemory() / (float) m_Tracker.GetCommittedMemory(); }

		void* Allocate();

		bool Free(void* address);

		size_t PageSize() const { return m_Tracker.m_PageSize; }

		bool IsAllocated(void* address) const;

		const MemoryRange& Range() const { _ASSERT(m_Memory); return m_Memory->Range(); }

	private:

		bool CommitMemory();

		static constexpr float kStartDecommitThreshold	= .4f; 
		
		static constexpr float kEndDecommitThreshold	= .8f;

		struct CommittedMemoryTracker
		{
			CommittedMemoryTracker() = default;

			CommittedMemoryTracker(MemoryRange range, size_t block_size);

			void OnCommit(void* address);

			void OnFree(void* address);

			bool IsCommitted(void* address) const;

			void* GetNextPageToCommit() const;

			size_t GetCommittedMemory() const { return m_CommittedPages * m_PageSize; }

			size_t PageNumber(void* address) const;

			MemoryRange m_AddressRange;

			size_t m_PageSize = 0;

			size_t m_CommittedPages = 0;

			Vector<uint64_t> m_CommittedPagesMasks;	// Each value is a mask that tells for each bit, if that block is committed. (bit == block)

			Vector<uint64_t> m_CommittedIndexMasks; // Each value is a mask that tells for each bit, if that mask if committed. (bit == mask)

			static constexpr size_t kMaskSize = sizeof(uint64_t) * 8;

			static constexpr uint64_t kFullCommittedMask = 0xFFFFFFFFFFFFFFFF;
		};

		SharedPtr<VirtualMemoryRegion> m_Memory;

		CommittedMemoryTracker m_Tracker;

		size_t m_AllocatedPages = 0;

		void* m_NextFreePage = nullptr;

		struct FreePage
		{
			FreePage(void* nextPage)
				: m_Pattern(kFreePagePattern)
				, m_Next(nextPage)
			{}

			uint64_t m_Pattern;
			void* m_Next;

			bool IsValid() const { return m_Pattern == kFreePagePattern; }
		};		
	};
}
