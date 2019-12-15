#include "Core/Memory/Allocators/MultipoolAllocator.h"
#include "Core/Memory/VirtualMemory.h"

DEFINE_STATIC_LOG_CATEGORY(LogMultipoolAllocator, ELogVerbosity::Log);

namespace Zn
{
	MultipoolAllocator::MultipoolAllocator()
		: m_MinAllocationSize(VirtualMemory::GetPageSize())
		, m_Pools()
	{
		m_Pools.reserve(1);
		m_Pools.emplace_back(MemoryPool(m_MinAllocationSize));
	}

	MultipoolAllocator::MultipoolAllocator(size_t pools_num, size_t pool_address_space, size_t min_allocation_size)
		: m_MinAllocationSize(min_allocation_size)
		, m_Pools()
	{
		m_Pools.reserve(pools_num);
		for (size_t Index = 0; Index < pools_num; Index++)
		{
			m_Pools.emplace_back(MemoryPool(pool_address_space, min_allocation_size << Index));
		}
	}

	void* MultipoolAllocator::Allocate(size_t size, size_t alignment)
	{
		size_t AlignedSize = Memory::Align(size, alignment);

		MemoryPool* pPool = nullptr;

		for (auto& Pool : m_Pools)
		{
			if (AlignedSize <= Pool.BlockSize())
			{
				pPool = &Pool;
				break;
			}
		}

		_ASSERT(pPool);

		ZN_LOG(LogMultipoolAllocator, ELogVerbosity::Verbose, "Requesting allocation of size \t%i. Pool block size: \t%i.", size, pPool->BlockSize());

		return pPool->Allocate();
	}

	bool MultipoolAllocator::Free(void* address)
	{
		for (auto& Pool : m_Pools)
		{
			if (Pool.IsAllocated(address))
			{
				return Pool.Free(address);
			}
		}

		return false;
	}

	size_t MultipoolAllocator::GetMaxAllocationSize() const
	{
		return (m_Pools.end() - 1)->BlockSize();
	}
}