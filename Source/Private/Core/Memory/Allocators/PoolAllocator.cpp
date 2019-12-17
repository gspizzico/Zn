#include "Core/Memory/Allocators/PoolAllocator.h"
#include "Core/Memory/Memory.h"
#include <algorithm>

DEFINE_STATIC_LOG_CATEGORY(LogPoolAllocator, ELogVerbosity::Log)

namespace Zn
{
	MemoryPool::MemoryPool(size_t poolSize, size_t block_size)
		: m_Memory(nullptr)
		, m_AllocatedBlocks(0)
		, m_NextFreeBlock(nullptr)
		, m_Tracker()
	{
		size_t RangeSize = VirtualMemory::AlignToPageSize(poolSize);
		m_Memory = std::make_shared<VirtualMemoryRegion>(RangeSize);
		m_Tracker = CommittedMemoryTracker(Range(), block_size);
		m_NextFreeBlock = Range().Begin();
	}

	MemoryPool::MemoryPool(size_t block_size)
		: MemoryPool(Memory::GetMemoryStatus().m_TotalPhys, block_size)
	{
	}

	MemoryPool::MemoryPool(SharedPtr<VirtualMemoryRegion> region, size_t block_size)
		: m_Memory(region)
		, m_Tracker(Range(), VirtualMemory::AlignToPageSize(block_size))
		, m_AllocatedBlocks(0)
		, m_NextFreeBlock(m_Tracker.GetNextPageToCommit())
	{	
	}

	void* MemoryPool::Allocate()
	{
		if(!m_Tracker.IsCommitted(m_NextFreeBlock))		// Check if we need to commit memory
		{
			if (!CommitMemory())
			{
				return nullptr;
			}
		}

		FreeBlock* BlockAddress = reinterpret_cast<FreeBlock*>(m_NextFreeBlock);

		m_NextFreeBlock = BlockAddress->m_Next;										// At the block address there is the address of the next free block
		
		if (m_NextFreeBlock == nullptr)												// If 0, then it means that the next free block is contiguous to this block.
		{
			m_NextFreeBlock = m_Tracker.GetNextPageToCommit();
		}

		MemoryDebug::MarkUninitialized(BlockAddress, Memory::AddOffset(BlockAddress, BlockSize()));

		m_AllocatedBlocks++;

		return BlockAddress;
	}

	bool MemoryPool::Free(void* address)
	{
		_ASSERT(Range().Contains(address));
		_ASSERT(address == Memory::AlignToAddress(address, Range().Begin(), BlockSize()));

		MemoryDebug::MarkFree(address, Memory::AddOffset(address, BlockSize()));

		m_NextFreeBlock = new(address) FreeBlock(m_NextFreeBlock);					// Write at the freed block, the address of the current free block. The current free block it's the freed block

		m_AllocatedBlocks--;

		if (GetMemoryUtilization() < kStartDecommitThreshold)						// Attempt to decommit some blocks since mem utilization is low
		{
			ZN_LOG(LogPoolAllocator, ELogVerbosity::Verbose, "Memory utilization %.2f, decommitting some pages.", GetMemoryUtilization());
			
			while (m_Tracker.IsCommitted(m_NextFreeBlock) && GetMemoryUtilization() < kEndDecommitThreshold)
			{
				FreeBlock* ToFree = reinterpret_cast<FreeBlock*>(m_NextFreeBlock);
				_ASSERT(ToFree->IsValid());

				m_NextFreeBlock = ToFree->m_Next;
				
				_ASSERT(Range().Contains(m_NextFreeBlock));

				ZN_LOG(LogPoolAllocator, ELogVerbosity::Verbose, "%p \t %x \t %p"
					, ToFree
					, *reinterpret_cast<uint64_t*>(ToFree)
					, m_NextFreeBlock);

				ZN_VM_CHECK(VirtualMemory::Decommit(ToFree, BlockSize()));

				m_Tracker.OnFree(ToFree);
			}

			if (!m_Tracker.IsCommitted(m_NextFreeBlock))
			{
				m_NextFreeBlock = m_Tracker.GetNextPageToCommit();
			}
		}

		return true;
	}

	bool MemoryPool::IsAllocated(void* address) const
	{
		if (!Range().Contains(address)) return false;

		FreeBlock* PageAddress = reinterpret_cast<FreeBlock*>(Memory::AlignToAddress(address, Range().Begin(), BlockSize()));

		if(m_Tracker.IsCommitted(PageAddress))
		{
			return !PageAddress->IsValid();
		}
		else
		{
			return false;
		}
	}

	bool MemoryPool::CommitMemory()
	{
		const bool CommitResult = Range().Contains(m_NextFreeBlock) ? VirtualMemory::Commit(m_NextFreeBlock, BlockSize()) : false;
		if (CommitResult)
		{
			m_Tracker.OnCommit(m_NextFreeBlock);						// Keep track of committed pages.
		}

		return CommitResult;
	}

	MemoryPool::CommittedMemoryTracker::CommittedMemoryTracker(MemoryRange range, size_t block_size)
		: m_AddressRange(range)
		, m_BlockSize(VirtualMemory::AlignToPageSize(block_size))
		, m_CommittedBlocks(0)
	{
		uint64_t NumBlocks = m_AddressRange.Size() / m_BlockSize;		// Number of allocable blocks.
		uint64_t NumMasks = NumBlocks / (kMaskSize);					// Number of masks. Each mask covers kMaskSize (64) blocks.

		m_CommittedIndexMasks.resize(NumMasks / kMaskSize);
		m_CommittedPagesMasks.resize(NumMasks);
	}

	void MemoryPool::CommittedMemoryTracker::OnCommit(void* address)
	{	
		size_t BlockNum = BlockNumber(address);

		size_t PagesMaskIndex = BlockNum / (kMaskSize);					// Index of mask containing committed pages
		size_t PageIndex = BlockNum % (kMaskSize);						// Index of page in 64bit mask
		
		m_CommittedPagesMasks[PagesMaskIndex] |= (1ull << PageIndex);

		bool IsFullyCommitted = (m_CommittedPagesMasks[PagesMaskIndex] == kFullCommittedMask);

		if (IsFullyCommitted)											// If fully committed, flag this mask as full.
		{
			size_t CIMIndex = PagesMaskIndex / (kMaskSize);
			size_t CIMValue = PagesMaskIndex % (kMaskSize);
			
			m_CommittedIndexMasks[CIMIndex] |= (1ull << CIMValue);
		}

		m_CommittedBlocks++;
	}

	void MemoryPool::CommittedMemoryTracker::OnFree(void* address)
	{	
		_ASSERT(m_AddressRange.Contains(address));

		size_t BlockNum = BlockNumber(address);

		size_t PagesMaskIndex = BlockNum / (kMaskSize);					// Index of mask containing committed pages
		size_t PageIndex = BlockNum % (kMaskSize);						// Index of page in 64bit mask

		m_CommittedPagesMasks[PagesMaskIndex] &= ~(1ull << PageIndex);

		size_t CIMIndex = PagesMaskIndex / (kMaskSize);
		size_t CIMValue = PagesMaskIndex % (kMaskSize);

		m_CommittedIndexMasks[CIMIndex] &= ~(1ull << CIMValue);

		m_CommittedBlocks--;
	}

	bool MemoryPool::CommittedMemoryTracker::IsCommitted(void* address) const
	{
		if (!m_AddressRange.Contains(address)) return false;

		size_t BlockNum = BlockNumber(address);

		size_t PagesMaskIndex = BlockNum / (kMaskSize);					// Index of mask containing committed pages
		size_t PageIndex = BlockNum % (kMaskSize);						// Index of page in 64bit mask

		const uint64_t BitValue = (1ull << PageIndex);

		return (m_CommittedPagesMasks[PagesMaskIndex] & BitValue) == BitValue;
	}

	void* MemoryPool::CommittedMemoryTracker::GetNextPageToCommit() const
	{
		if (GetCommittedMemory() == m_AddressRange.Size()) return nullptr;

		for (size_t i = 0; i < m_CommittedIndexMasks.size(); ++i)
		{
			const auto& Mask = m_CommittedIndexMasks[i];

			if (Mask != kFullCommittedMask)
			{
				unsigned long PagesMaskIndex;
				_BitScanForward64(&PagesMaskIndex, ~Mask);

				PagesMaskIndex += static_cast<unsigned long>(i * kMaskSize);

				unsigned long BlockIndex;
				_BitScanForward64(&BlockIndex, ~m_CommittedPagesMasks[PagesMaskIndex]); 	// Biased towards lower addresses.

				void* MaskStartAddress = Memory::AddOffset(m_AddressRange.Begin(), (m_BlockSize * PagesMaskIndex * kMaskSize));
				return Memory::AddOffset(MaskStartAddress, m_BlockSize * BlockIndex);
			}
		}

		return nullptr;
	}

	size_t MemoryPool::CommittedMemoryTracker::BlockNumber(void* address) const
	{
		_ASSERT(m_AddressRange.Contains(address));

		auto Distance = Memory::GetDistance(address, m_AddressRange.Begin());				// Distance from memory range begin
		return static_cast<size_t>(float(Distance) / float(m_BlockSize));					// Index of block in 1 dimensional space
	}
}