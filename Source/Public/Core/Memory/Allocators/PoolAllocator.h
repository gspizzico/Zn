#pragma once
#include "Core/Memory/VirtualMemory.h"
#include "Core/Containers/Set.h"

namespace Zn
{
	class MemoryPool
	{
	public:

		static constexpr uint64_t kFreeBlockPattern = 0xfb;

		MemoryPool(size_t pool_size, size_t block_size);

		MemoryPool(size_t block_size);

		MemoryPool(SharedPtr<VirtualMemoryRegion> region, size_t block_size);

		size_t GetUsedMemory() const { return m_AllocatedBlocks * BlockSize(); }

		float GetMemoryUtilization() const { return (float)GetUsedMemory() / (float) m_Tracker.GetCommittedMemory(); }

		void* Allocate();

		bool Free(void* address);

		size_t BlockSize() const { return m_Tracker.m_BlockSize; }

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

			size_t GetCommittedMemory() const { return m_CommittedBlocks * m_BlockSize; }

			size_t BlockNumber(void* address) const;

			MemoryRange m_AddressRange;

			size_t m_BlockSize = 0;

			size_t m_CommittedBlocks = 0;

			Vector<uint64_t> m_CommittedPagesMasks;	// Each value is a mask that tells for each bit, if that block is committed. (bit == block)

			Vector<uint64_t> m_CommittedIndexMasks; // Each value is a mask that tells for each bit, if that mask if committed. (bit == mask)

			static constexpr size_t kMaskSize = sizeof(uint64_t) * 8;

			static constexpr uint64_t kFullCommittedMask = 0xFFFFFFFFFFFFFFFF;
		};

		SharedPtr<VirtualMemoryRegion> m_Memory;

		CommittedMemoryTracker m_Tracker;

		size_t m_AllocatedBlocks = 0;

		void* m_NextFreeBlock = nullptr;

		struct FreeBlock
		{
			FreeBlock(void* nextBlock)
				: m_Pattern(kFreeBlockPattern)
				, m_Next(nextBlock)
			{}

			uint64_t m_Pattern;
			void* m_Next;

			bool IsValid() const { return m_Pattern == kFreeBlockPattern; }
		};		
	};
}
