#include "Core/Memory/Allocators/PoolAllocator.h"
#include "Core/Memory/Memory.h"
#include <algorithm>

DEFINE_STATIC_LOG_CATEGORY(LogPoolAllocator, ELogVerbosity::Log)

namespace Zn
{
	MemoryPool::MemoryPool(size_t poolSize, size_t blockSize, size_t alignment)
		: m_Memory(poolSize)
		, m_BlockSize(Memory::Align(blockSize, VirtualMemory::GetPageSize()))
		, m_CommittedMemory(0)
		, m_AllocatedBlocks(0)
		, m_NextFreeBlock(m_Memory.Begin())
		, m_NextPage(m_Memory.Begin())
	{	
		CommitMemory();								// Commit an initial set of memory. Eventually it's going to be used.
	}

	MemoryPool::MemoryPool(size_t blockSize, size_t alignment)
		: MemoryPool(Memory::GetMemoryStatus().m_TotalPhys, blockSize, alignment)
	{
	}

	void* MemoryPool::Allocate()
	{
		if (static_cast<int64>(Memory::GetDistance(m_NextPage, m_NextFreeBlock)) < static_cast<int64>(m_BlockSize))		// Check if we need to commit memory
		{
			if (!CommitMemory())
			{
				return nullptr;
			}
		}

		auto BlockAddress = m_NextFreeBlock;

		m_NextFreeBlock = reinterpret_cast<void*>(*reinterpret_cast<uintptr_t*>(Memory::AddOffset(BlockAddress, sizeof(kFreeBlockPattern))));								// At the block address there is the address of the next free block
		
		if (m_NextFreeBlock == nullptr)																						// If 0, then it means that the next free block is contiguous to this block.
		{
			m_NextFreeBlock = Memory::AddOffset(BlockAddress, m_BlockSize);
		}

		MemoryDebug::MarkUninitialized(BlockAddress, Memory::AddOffset(BlockAddress, m_BlockSize));

		m_AllocatedBlocks++;

		return BlockAddress;
	}

	bool MemoryPool::Free(void* address)
	{
		_ASSERT(m_Memory.Range().Contains(address));

		MemoryDebug::MarkFree(address, Memory::AddOffset(address, m_BlockSize));

		auto& AtAddress = *reinterpret_cast<uintptr_t*>(address);

		AtAddress = kFreeBlockPattern;

		auto& AtNextAddress = *reinterpret_cast<uintptr_t*>(Memory::AddOffset(address, sizeof(kFreeBlockPattern)));

		AtNextAddress = reinterpret_cast<uintptr_t>(m_NextFreeBlock);														// Write at the freed block, the address of the current free block

		m_NextFreeBlock = address;																							// The current free block it's the freed block

		m_AllocatedBlocks--;

		if (m_NextFreeBlock != nullptr && GetMemoryUtilization() < kStartDecommitThreshold)									// Attempt to decommit some blocks since mem utilization is low
		{
			ZN_LOG(LogPoolAllocator, ELogVerbosity::Verbose, "Memory utilization %.2f, decommitting some pages.", GetMemoryUtilization());

			while (m_NextFreeBlock != nullptr && m_NextFreeBlock != m_NextPage && GetMemoryUtilization() < kEndDecommitThreshold)
			{
				auto ToFree = m_NextFreeBlock;

				m_NextFreeBlock = reinterpret_cast<void*>(*(uintptr_t*) Memory::AddOffset(m_NextFreeBlock, sizeof(kFreeBlockPattern)));

				ZN_VM_CHECK(VirtualMemory::Decommit(ToFree, m_BlockSize));

				if (auto It = m_CommittedPages.find(uintptr_t(ToFree)); It != m_CommittedPages.end())
				{
					m_CommittedPages.erase(It);
				}
			}
		}

		return true;
	}

	bool MemoryPool::IsAllocated(void* address) const
	{
		if (!m_Memory.Range().Contains(address)) return false;

		auto PageAddress = Memory::AlignDown(address, BlockSize());

		if(m_CommittedPages.count(uintptr_t(PageAddress)))
		{
			return *(uint64_t*)(PageAddress) != kFreeBlockPattern;
		}
		else
		{
			return false;
		}
	}

	bool MemoryPool::CommitMemory()
	{
		auto NextPageAddress = Memory::AddOffset(m_NextPage, m_BlockSize);

		_ASSERT(m_Memory.Range().Contains(NextPageAddress));

		const bool CommitResult = VirtualMemory::Commit(m_NextPage, m_BlockSize);
		if (CommitResult)
		{
			m_CommittedPages.emplace((uintptr_t)m_NextPage);
			m_NextPage = NextPageAddress;
			m_CommittedMemory += m_BlockSize;
		}

		return CommitResult;
	}
}