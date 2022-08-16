#include <Znpch.h>
#include "Core/Memory/Allocators/PageAllocator.h"
#include "Core/Memory/Memory.h"
#include <algorithm>

DEFINE_STATIC_LOG_CATEGORY(LogPoolAllocator, ELogVerbosity::Log)

namespace Zn
{
	PageAllocator::PageAllocator(size_t poolSize, size_t page_size)
		: m_Memory(VirtualMemory::AlignToPageSize(poolSize))
		, m_AllocatedPages(0)
		, m_NextFreePage(nullptr)
		, m_Tracker()
	{
		m_Tracker = CommittedMemoryTracker(Range(), page_size);
		m_NextFreePage = Range().Begin();
	}

	PageAllocator::PageAllocator(size_t page_size)
		: PageAllocator(Memory::GetMemoryStatus().m_TotalPhys, page_size)
	{}

	void* PageAllocator::Allocate()
	{
		if (!m_Tracker.IsCommitted(m_NextFreePage))		// Check if we need to commit memory
		{
			if (!CommitMemory())
			{
				return nullptr;
			}
		}

		FreePage* PageAddress = reinterpret_cast<FreePage*>(m_NextFreePage);

		m_NextFreePage = PageAddress->m_Next;										// At the page address there is the address of the next free page

		if (m_NextFreePage == nullptr)												// If 0, then it means that the next free page is contiguous to this page.
		{
			m_NextFreePage = m_Tracker.GetNextPageToCommit();
		}

		MemoryDebug::MarkUninitialized(PageAddress, Memory::AddOffset(PageAddress, PageSize()));

		m_AllocatedPages++;

		return PageAddress;
	}

	bool PageAllocator::Free(void* address)
	{
		_ASSERT(Range().Contains(address));
		_ASSERT(address == Memory::AlignToAddress(address, Range().Begin(), PageSize()));

		MemoryDebug::MarkFree(address, Memory::AddOffset(address, PageSize()));

		m_NextFreePage = new(address) FreePage(m_NextFreePage);					// Write at the freed page, the address of the current free page. The current free page it's the freed page

		m_AllocatedPages--;

		if (GetMemoryUtilization() < kStartDecommitThreshold)						// Attempt to decommit some pages since mem utilization is low
		{
			ZN_LOG(LogPoolAllocator, ELogVerbosity::Verbose, "Memory utilization %.2f, decommitting some pages.", GetMemoryUtilization());

			while (m_Tracker.IsCommitted(m_NextFreePage) && GetMemoryUtilization() < kEndDecommitThreshold)
			{
				FreePage* ToFree = reinterpret_cast<FreePage*>(m_NextFreePage);
				_ASSERT(ToFree->IsValid());

				m_NextFreePage = ToFree->m_Next;

				_ASSERT(Range().Contains(m_NextFreePage));

				ZN_LOG(LogPoolAllocator, ELogVerbosity::Verbose, "%p \t %x \t %p"
					, ToFree
					, *reinterpret_cast<uint64_t*>(ToFree)
					, m_NextFreePage);

				ZN_VM_CHECK(VirtualMemory::Decommit(ToFree, PageSize()));

				m_Tracker.OnFree(ToFree);
			}

			if (!m_Tracker.IsCommitted(m_NextFreePage))
			{
				m_NextFreePage = m_Tracker.GetNextPageToCommit();
			}
		}

		return true;
	}

	bool PageAllocator::IsAllocated(void* address) const
	{
		if (FreePage* PageAddress = reinterpret_cast<FreePage*>(GetPageAddress(address)))
		{
			return m_Tracker.IsCommitted(PageAddress) && !PageAddress->IsValid();
		}

		return false;
	}

	void* PageAllocator::GetPageAddress(void* address) const
	{
		if (Range().Contains(address))
		{
			auto StartAddress = Range().Begin();

			auto PageIndex = m_Tracker.PageNumber(address);

			return Memory::AddOffset(StartAddress, PageSize() * PageIndex);
		}

		return nullptr;
	}

	bool PageAllocator::CommitMemory()
	{
		const bool CommitResult = Range().Contains(m_NextFreePage) ? VirtualMemory::Commit(m_NextFreePage, PageSize()) : false;
		if (CommitResult)
		{
			m_Tracker.OnCommit(m_NextFreePage);						// Keep track of committed pages.
		}

		return CommitResult;
	}

	PageAllocator::CommittedMemoryTracker::CommittedMemoryTracker(MemoryRange range, size_t page_size)
		: m_AddressRange(range)
		, m_PageSize(VirtualMemory::AlignToPageSize(page_size))
		, m_PageSizeMSB(0)
		, m_CommittedPages(0)
	{
		uint64_t NumPages = m_AddressRange.Size() / m_PageSize;		// Number of allocable pages.
		uint64_t NumMasks = NumPages / (kMaskSize);					// Number of masks. Each mask covers kMaskSize (64) pages.

		m_CommittedIndexMasks.resize(NumMasks / kMaskSize);
		m_CommittedPagesMasks.resize(NumMasks);

		unsigned long MSB;
		_BitScanReverse64(&MSB, m_PageSize);

		m_PageSizeMSB = MSB;
	}

	void PageAllocator::CommittedMemoryTracker::OnCommit(void* address)
	{
		size_t PageNum = PageNumber(address);

		size_t PagesMaskIndex = PageNum / (kMaskSize);					// Index of mask containing committed pages
		size_t PageIndex = PageNum % (kMaskSize);						// Index of page in 64bit mask

		m_CommittedPagesMasks[PagesMaskIndex] |= (1ull << PageIndex);

		bool IsFullyCommitted = (m_CommittedPagesMasks[PagesMaskIndex] == kFullCommittedMask);

		if (IsFullyCommitted)											// If fully committed, flag this mask as full.
		{
			size_t CIMIndex = PagesMaskIndex / (kMaskSize);
			size_t CIMValue = PagesMaskIndex % (kMaskSize);

			m_CommittedIndexMasks[CIMIndex] |= (1ull << CIMValue);
		}

		m_CommittedPages++;
	}

	void PageAllocator::CommittedMemoryTracker::OnFree(void* address)
	{
		_ASSERT(m_AddressRange.Contains(address));

		size_t PageNum = PageNumber(address);

		size_t PagesMaskIndex = PageNum / (kMaskSize);					// Index of mask containing committed pages
		size_t PageIndex = PageNum % (kMaskSize);						// Index of page in 64bit mask

		m_CommittedPagesMasks[PagesMaskIndex] &= ~(1ull << PageIndex);

		size_t CIMIndex = PagesMaskIndex / (kMaskSize);
		size_t CIMValue = PagesMaskIndex % (kMaskSize);

		m_CommittedIndexMasks[CIMIndex] &= ~(1ull << CIMValue);

		m_CommittedPages--;
	}

	bool PageAllocator::CommittedMemoryTracker::IsCommitted(void* address) const
	{
		if (!m_AddressRange.Contains(address)) return false;

		size_t PageNum = PageNumber(address);

		size_t PagesMaskIndex = PageNum / (kMaskSize);					// Index of mask containing committed pages
		size_t PageIndex = PageNum % (kMaskSize);						// Index of page in 64bit mask

		const uint64_t BitValue = (1ull << PageIndex);

		return (m_CommittedPagesMasks[PagesMaskIndex] & BitValue) == BitValue;
	}

	void* PageAllocator::CommittedMemoryTracker::GetNextPageToCommit() const
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

				unsigned long PageIndex;
				_BitScanForward64(&PageIndex, ~m_CommittedPagesMasks[PagesMaskIndex]); 	// Biased towards lower addresses.

				void* MaskStartAddress = Memory::AddOffset(m_AddressRange.Begin(), (m_PageSize * PagesMaskIndex * kMaskSize));
				return Memory::AddOffset(MaskStartAddress, m_PageSize * PageIndex);
			}
		}

		return nullptr;
	}

	size_t PageAllocator::CommittedMemoryTracker::PageNumber(void* address) const
	{
		_ASSERT(m_AddressRange.Contains(address));

		auto Distance = Memory::GetDistance(address, m_AddressRange.Begin());				// Distance from memory range begin

		return Distance >> m_PageSizeMSB;													// Index of page in 1 dimensional space
	}
}