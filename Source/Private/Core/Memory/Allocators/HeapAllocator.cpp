#include "Core/Memory/Allocators/HeapAllocator.h"
#include "Core/Memory/Memory.h"
#include "Core/Log/LogMacros.h"

DECLARE_STATIC_LOG_CATEGORY(LogHeapAllocator, ELogVerbosity::Log);

namespace Zn
{
	HeapAllocator::HeapAllocator()
		: m_MemoryHeap(kDefaultRegionSize)
		, m_PageSize(kDefaultPageSize)
		, m_RegionIndex(0)
		, m_Address(m_MemoryHeap.GetRegion(0))
	{
	}

	HeapAllocator::HeapAllocator(size_t memory_region_size, size_t page_size)
		: m_MemoryHeap(memory_region_size)
		, m_PageSize(page_size)
		, m_RegionIndex(0)
		, m_Address(m_MemoryHeap.GetRegion(0))
	{
	}

	HeapAllocator::HeapAllocator(VirtualMemoryHeap&& heap, size_t page_size)
		: m_MemoryHeap(std::move(heap))
		, m_PageSize(page_size)
		, m_RegionIndex(m_MemoryHeap.Regions().size() - 1)
		, m_Address(m_MemoryHeap.GetRegion(m_RegionIndex))
	{
	}

	HeapAllocator::HeapAllocator(HeapAllocator&& other)
		: m_MemoryHeap(std::move(other.m_MemoryHeap))
		, m_PageSize(other.m_PageSize)
		, m_RegionIndex(other.m_RegionIndex)
		, m_Address(other.m_Address)
	{
		other.m_PageSize = 0;
		other.m_RegionIndex = 0;
		other.m_Address = nullptr;
	}

	HeapAllocator::~HeapAllocator()
	{
	}

	void* HeapAllocator::AllocatePage()
	{
		_ASSERT(m_PageSize > 0);

		auto RegionStartAddress = m_MemoryHeap.GetRegion(m_RegionIndex);

		auto RegionEndAddress = Memory::AddOffset(RegionStartAddress, m_MemoryHeap.GetRegionSize());

		//if (Memory::AddOffset(m_Address, m_PageSize) >= RegionEndAddress)
		if(m_Address == RegionEndAddress)
		{
			m_Address = m_MemoryHeap.AllocateRegion();
			m_RegionIndex++;
		}

		auto PageAddress = m_Address;

		m_Address = Memory::AddOffset(m_Address, m_PageSize);

		ZN_LOG(LogHeapAllocator, ELogVerbosity::Log, "Committing %i bytes on region %i. %i bytes left. \t Total committed memory: %i bytes. \t Region committed memory %i bytes", m_PageSize, m_RegionIndex
			, Memory::GetDistance(Memory::AddOffset(m_MemoryHeap.GetRegion(m_RegionIndex), m_MemoryHeap.GetRegionSize()), m_Address)	// Region Last Address - Next Page Address
			, (m_MemoryHeap.GetRegionSize() * m_RegionIndex + Memory::GetDistance(m_Address, m_MemoryHeap.GetRegion(m_RegionIndex)))	// Region Size * (Regions Num - 1) + (Next Page Address - Region Start Address)
			, Memory::GetDistance(m_Address, m_MemoryHeap.GetRegion(m_RegionIndex)));													// Next Page Address - Region Start Address
		
		ZN_VM_CHECK(VirtualMemory::Commit(PageAddress, m_PageSize));

		MemoryDebug::MarkUninitialized(PageAddress, m_Address);

		return PageAddress;
	}

	bool HeapAllocator::Free(void* address)
	{
		size_t RegionIndex = 0;
		
		if (m_MemoryHeap.GetRegionIndex(RegionIndex, address))
		{
			auto Region = m_MemoryHeap.Regions()[RegionIndex];
			if (**Region != address)
			{
				ZN_LOG(LogHeapAllocator, ELogVerbosity::Error, "Cannot free address %p because it's not a region address.", address);
				return false;
			}

			return m_MemoryHeap.FreeRegion(RegionIndex);
		}

		return false;
	}
}
