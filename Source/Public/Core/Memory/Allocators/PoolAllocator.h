#pragma once
#include "Core/Memory/VirtualMemory.h"

namespace Zn
{
	class MemoryPool
	{
	public:

		MemoryPool(size_t poolSize, size_t blockSize, size_t alignment = sizeof(uintptr_t));

		MemoryPool(size_t blockSize, size_t alignment = sizeof(uintptr_t));

		size_t GetUsedMemory() const { return m_AllocatedBlocks * m_BlockSize; }

		float GetMemoryUtilization() const { return (float)GetUsedMemory() / (float) m_CommittedMemory; }

		void* Allocate();

		bool Free(void* address);

		size_t BlockSize() const { return m_BlockSize; }

	private:

		bool CommitMemory();

		static constexpr float kStartDecommitThreshold	= .4f;
		
		static constexpr float kEndDecommitThreshold	= .8f;

		VirtualMemoryRegion m_Memory;

		size_t m_BlockSize;

		size_t m_CommittedMemory;

		size_t m_AllocatedBlocks;

		void* m_NextFreeBlock;
		
		void* m_NextPage;
	};
}
