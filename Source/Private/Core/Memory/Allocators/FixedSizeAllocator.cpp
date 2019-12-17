#include "Core/Memory/Allocators/FixedSizeAllocator.h"
#include "Core/Memory/Memory.h"
#include "Core/Log/LogMacros.h"
#include <algorithm>

DEFINE_STATIC_LOG_CATEGORY(LogFixedSizeAllocator, ELogVerbosity::Log);

namespace Zn
{
	FixedSizeAllocator::FixedSizeAllocator(size_t allocationSize, SharedPtr<MemoryPool> memoryPool)
		: m_MemoryPool(memoryPool)
		, m_AllocationSize(std::max(Memory::Align(allocationSize, 2), kMinAllocationSize))
		, m_NextFreeBlock(nullptr)
		, m_FreePageList()
		, m_FullPageList()
	{	
	}

	FixedSizeAllocator::FixedSizeAllocator(size_t allocationSize, size_t pageSize)
		: FixedSizeAllocator(allocationSize, nullptr)
	{
		m_MemoryPool = std::make_shared<MemoryPool>(VirtualMemory::AlignToPageSize(pageSize));
	}

	void* FixedSizeAllocator::Allocate()
	{
		if (m_FreePageList.size() == 0)
		{
			AllocatePage();
		}

		auto Top = m_FreePageList.front();

		auto CurrentPage = reinterpret_cast<FSAPage*>(Top);

		auto Block = CurrentPage->Allocate();

		if (CurrentPage->IsFull())
		{
			m_FreePageList.pop_front();
			m_FullPageList.emplace(reinterpret_cast<uintptr_t>(CurrentPage));
		}

		return Block;		
	}

	void FixedSizeAllocator::Free(void* address)
	{
		auto PageAddress = FSAPage::GetPageFromAnyAddress(address, m_MemoryPool->Range().Begin(), m_MemoryPool->BlockSize());
		
		_ASSERT(PageAddress != NULL && PageAddress->m_AllocationSize == m_AllocationSize);

		PageAddress->Free(address);
		
		uintptr_t PageKey = reinterpret_cast<uintptr_t>(PageAddress);

		if (PageAddress->m_AllocatedBlocks == PageAddress->MaxAllocations() - 1)
		{
			ZN_LOG(LogFixedSizeAllocator, ELogVerbosity::Verbose, "A slot on page %i has been freed. This page is added back to the FreePageList.", PageKey);

			m_FreePageList.push_back(PageKey);
			m_FullPageList.erase(PageKey);
		}
		else if (PageAddress->m_AllocatedBlocks == 0)
		{
			m_FreePageList.remove(PageKey);

			m_MemoryPool->Free(PageAddress);

			ZN_LOG(LogFixedSizeAllocator, ELogVerbosity::Verbose, "Page %i is empty. Freeing it", PageKey);
		}
	}

	void FixedSizeAllocator::AllocatePage()
	{
		if (m_MemoryPool)
		{
			auto NewPage = new (m_MemoryPool->Allocate()) FSAPage(m_MemoryPool->BlockSize(), m_AllocationSize);

			auto PageKey = reinterpret_cast<uintptr_t>(NewPage);

			m_FreePageList.push_back(PageKey);

			ZN_LOG(LogFixedSizeAllocator, ELogVerbosity::Verbose, "Requested a page of size \t%i from the pool.", m_MemoryPool->BlockSize());
		}
	}	

	FixedSizeAllocator::FSAPage::FSAPage(size_t page_size, size_t allocation_size)
		: m_Size(page_size)
		, m_AllocationSize(allocation_size)
		, m_AllocatedBlocks(0)
		, m_NextFreeBlock(StartAddress())
	{
		const auto NumBlocks = MaxAllocations();

		const auto PageHeaderSize = Memory::GetDistance(StartAddress(), this);

		_ASSERT(NumBlocks - 1 <= (size_t)std::numeric_limits<uint16_t>::max());

		for (uint16_t BlockIndex = 0; BlockIndex < NumBlocks; BlockIndex++)					// for every block, create a free block which points to the next one.
		{
			const auto Offset = PageHeaderSize + (BlockIndex * m_AllocationSize);

			auto BlockAddress = Memory::AddOffset(this, Offset);

			uint16_t NextBlockOffset = BlockIndex < NumBlocks - 1 ? (uint16_t)(Offset + m_AllocationSize) : std::numeric_limits<uint16_t>::max();

			new (BlockAddress) FreeBlock{ FreeBlock::kValidationToken, NextBlockOffset };
		}
	}

	bool FixedSizeAllocator::FSAPage::IsFull() const
	{	
		return MaxAllocations() == m_AllocatedBlocks;
	}

	size_t FixedSizeAllocator::FSAPage::MaxAllocations() const
	{
		return Memory::GetDistance(Memory::AddOffset(const_cast<FSAPage*>(this), m_Size), StartAddress()) / m_AllocationSize;
	}

	void* FixedSizeAllocator::FSAPage::Allocate()
	{
		auto Block = m_NextFreeBlock;

		if (m_NextFreeBlock != nullptr)
		{
			_ASSERT(m_NextFreeBlock->m_AllocationToken == FreeBlock::kValidationToken);

			m_NextFreeBlock = m_NextFreeBlock->m_NextBlockOffset != std::numeric_limits<uint16_t>::max()							// If the next block offset is valid, the next block ptr is replaced.
				? (FixedSizeAllocator::FreeBlock*) (Memory::AddOffset(this, m_NextFreeBlock->m_NextBlockOffset))
				: nullptr;

			m_AllocatedBlocks++;

			Memory::Memzero(Block, Memory::AddOffset(Block, sizeof(FreeBlock)));

			MemoryDebug::MarkUninitialized(Block, Memory::AddOffset(Block, m_AllocationSize));
		}

		return Block;
	}

	void FixedSizeAllocator::FSAPage::Free(void* address)
	{
		m_AllocatedBlocks--;

		MemoryDebug::MarkFree(address, Memory::AddOffset(address, m_AllocationSize));

		auto NewBlock = new (address) FreeBlock{ FreeBlock::kValidationToken, std::numeric_limits<uint16_t>::max() };

		if (m_NextFreeBlock != nullptr)
		{
			_ASSERT(m_NextFreeBlock->m_AllocationToken == FreeBlock::kValidationToken);

			auto Offset = Memory::GetDistance(m_NextFreeBlock, this);

			_ASSERT(Offset >= 0);

			NewBlock->m_NextBlockOffset = static_cast<uint32_t>(Offset);
		}

		m_NextFreeBlock = NewBlock;
	}

	inline FixedSizeAllocator::FSAPage* FixedSizeAllocator::FSAPage::GetPageFromAnyAddress(void* address, void* start_address, size_t page_size)
	{
		return reinterpret_cast<FSAPage*>(Memory::AlignToAddress(address, address, page_size));
	}

	inline FixedSizeAllocator::FreeBlock* FixedSizeAllocator::FSAPage::StartAddress() const
	{
		return reinterpret_cast<FixedSizeAllocator::FreeBlock*>(Memory::Align(Memory::AddOffset(const_cast<FSAPage*>(this), sizeof(FSAPage)), m_AllocationSize));
	}
}