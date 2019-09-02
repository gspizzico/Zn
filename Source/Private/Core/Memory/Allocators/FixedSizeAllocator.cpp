#include "Core/Memory/Allocators/FixedSizeAllocator.h"
#include "Core/Memory/Memory.h"
#include "Core/Log/LogMacros.h"
#include <algorithm>

DECLARE_STATIC_LOG_CATEGORY(LogFixedSizeAllocator, ELogVerbosity::Log);

namespace Zn
{
	FixedSizeAllocator::FixedSizeAllocator(size_t allocationSize, SharedPtr<MemoryPool> memoryPool)
		: m_MemoryPool(memoryPool)
		, m_AllocationSize(std::max(Memory::Align(allocationSize, 2), kMinAllocationSize))
		, m_NextFreeBlock(nullptr)
		, m_Pages()
		, m_FreePageList()
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

		auto& CurrentPage = m_Pages[Top];

		auto Block = CurrentPage.Allocate();

		const auto MaxAllocableSize = m_MemoryPool->BlockSize() - m_MemoryPool->BlockSize() % m_AllocationSize;

		if (CurrentPage.m_Page.AllocatedSize() == MaxAllocableSize)
		{
			m_FreePageList.pop_front();
		}

		return Block;		
	}

	void FixedSizeAllocator::Free(void* address)
	{
		auto PageAddress = Memory::AlignDown(address, m_MemoryPool->BlockSize());
		
		uintptr_t PageKey = reinterpret_cast<uintptr_t>(PageAddress);

		auto& Page = m_Pages[PageKey];
		Page.Free(address);

		if (Page.m_Page.AllocatedSize() == m_MemoryPool->BlockSize() - m_AllocationSize)
		{
			ZN_LOG(LogFixedSizeAllocator, ELogVerbosity::Verbose, "A slot on page %i has been freed. This page is added back to the FreePageList.", PageKey);
			m_FreePageList.push_back(PageKey);
		}
		else if (Page.m_Page.AllocatedSize() == 0)
		{
			m_FreePageList.remove(PageKey);

			auto PageAddress = Page.m_Page.Begin();

			m_Pages.erase(PageKey);

			m_MemoryPool->Free(PageAddress);

			ZN_LOG(LogFixedSizeAllocator, ELogVerbosity::Verbose, "Page %i is empty. Freeing it", PageKey);
		}
	}

	void FixedSizeAllocator::AllocatePage()
	{
		if (m_MemoryPool)
		{
			auto Range = MemoryRange{ m_MemoryPool->Allocate(), m_MemoryPool->BlockSize() };

			auto PageKey = reinterpret_cast<uintptr_t>(Range.Begin());

			auto AddedValue = m_Pages.try_emplace(PageKey, Page(Range, m_AllocationSize));

			m_FreePageList.push_back(PageKey);

			ZN_LOG(LogFixedSizeAllocator, ELogVerbosity::Verbose, "Requested a page of size \t%i from the pool.", m_MemoryPool->BlockSize());
		}
	}

	FixedSizeAllocator::Page::Page()
		: m_Page()
		, m_AllocationSize(0)
		, m_NextFreeBlock(nullptr)
	{
	}

	FixedSizeAllocator::Page::Page(MemoryRange pageRange, size_t allocationSize)
		: m_Page(pageRange)
		, m_AllocationSize(allocationSize)
		, m_NextFreeBlock(reinterpret_cast<FreeBlock*>(pageRange.Begin()))
	{
		const auto NumBlocks = pageRange.Size() / m_AllocationSize;

		_ASSERT(NumBlocks - 1 <= (size_t) std::numeric_limits<uint16_t>::max());

		for (uint16_t BlockIndex = 0; BlockIndex < NumBlocks; BlockIndex++)					// for every block, create a free block which points to the next one.
		{
			const auto Offset = BlockIndex * m_AllocationSize;

			auto BlockAddress = Memory::AddOffset(pageRange.Begin(), Offset);
			
			uint16_t NextBlockOffset = BlockIndex < NumBlocks - 1 ? (uint16_t)(Offset + m_AllocationSize) : std::numeric_limits<uint16_t>::max();

			new (BlockAddress) FreeBlock{ FreeBlock::kValidationToken, NextBlockOffset };
		}
	}

	FixedSizeAllocator::Page::Page(Page&& other) noexcept
		: m_Page(std::move(other.m_Page))
		, m_AllocationSize(other.m_AllocationSize)
		, m_NextFreeBlock(other.m_NextFreeBlock)
	{
		other.m_NextFreeBlock = nullptr;
	}

	void* FixedSizeAllocator::Page::Allocate()
	{
		auto Block = m_NextFreeBlock;

		if (m_NextFreeBlock != nullptr)
		{
			_ASSERT(m_NextFreeBlock->m_AllocationToken == FreeBlock::kValidationToken);

			m_NextFreeBlock = m_NextFreeBlock->m_NextBlockOffset != std::numeric_limits<uint16_t>::max()							// If the next block offset is valid, the next block ptr is replaced.
				? (FixedSizeAllocator::FreeBlock*) (Memory::AddOffset(m_Page.Begin(), m_NextFreeBlock->m_NextBlockOffset)) 
				: nullptr;
					
			m_Page.TrackAllocation(m_AllocationSize);

			Memory::Memzero(Block, Memory::AddOffset(Block, sizeof(FreeBlock)));
			
			MemoryDebug::MarkUninitialized(Block, Memory::AddOffset(Block, m_AllocationSize));
		}		

		return Block;
	}

	void FixedSizeAllocator::Page::Free(void* address)
	{
		m_Page.TrackFree(m_AllocationSize);

		uintptr_t BeforeFree = *reinterpret_cast<uintptr_t*>(address);

		MemoryDebug::MarkFree(address, Memory::AddOffset(address, m_AllocationSize));

		auto NewBlock = new (address) FreeBlock{ FreeBlock::kValidationToken, std::numeric_limits<uint16_t>::max() };

		if (m_NextFreeBlock != nullptr)
		{
			_ASSERT(m_NextFreeBlock->m_AllocationToken == FreeBlock::kValidationToken);

			NewBlock->m_NextBlockOffset = Memory::GetDistance(m_NextFreeBlock, m_Page.Begin());
		}

		m_NextFreeBlock = NewBlock;
	}	
}