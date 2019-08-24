#include "Core/Memory/Allocators/PoolAllocator.h"
#include <algorithm>

namespace Zn
{
	MemoryPool::MemoryPool(size_t blockSize, size_t alignment)
		: m_Memory(Memory::GetMemoryStatus().m_TotalPhys)
		, m_BlockSize(Memory::Align(blockSize, std::max(sizeof(uintptr_t), alignment)))
		, m_CommittedMemory(0)
		, m_MinMemoryCommitSize(Memory::Align(blockSize * kMinBlockNum, VirtualMemory::GetPageSize()))
		, m_AllocatedBlocks(0)
		, m_NextFreeBlock(m_Memory.Begin())
		, m_NextPage(m_Memory.Begin())
	{	
		VirtualMemory::Commit(m_NextPage, m_MinMemoryCommitSize);															// Commit an initial set of memory. Eventually it's going to be used.
		m_CommittedMemory += m_MinMemoryCommitSize;
		m_NextPage = Memory::AddOffset(m_NextPage, m_MinMemoryCommitSize);
	}

	void* MemoryPool::Allocate()
	{
		if (static_cast<int64>(Memory::GetDistance(m_NextPage, m_NextFreeBlock)) <= static_cast<int64>(m_BlockSize))		// Check if we need to commit memory
		{
			if (!CommitMemory())
			{
				return nullptr;
			}
		}

		auto BlockAddress = m_NextFreeBlock;

		m_NextFreeBlock = reinterpret_cast<void*>(*reinterpret_cast<uintptr_t*>(BlockAddress));								// At the block address there is the address of the next free block
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
		AtAddress = reinterpret_cast<uintptr_t>(m_NextFreeBlock);															// Write at the freed block, the address of the current free block

		m_NextFreeBlock = address;																							// The current free block it's the freed block

		m_AllocatedBlocks--;

		return true;
	}

	bool MemoryPool::CommitMemory()
	{
		auto NextPageAddress = Memory::AddOffset(m_NextPage, m_MinMemoryCommitSize);

		_ASSERT(m_Memory.Range().Contains(NextPageAddress));

		const bool CommitResult = VirtualMemory::Commit(m_NextPage, m_MinMemoryCommitSize);

		if (CommitResult)
		{
			m_NextPage = NextPageAddress;
			m_CommittedMemory += m_MinMemoryCommitSize;
		}

		return CommitResult;
	}
}