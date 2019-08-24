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
		, m_NextPageAddress(m_MemoryHeap.GetRegion(0)->Begin())
	{
	}

	HeapAllocator::HeapAllocator(size_t memory_region_size, size_t page_size)
		: m_MemoryHeap(memory_region_size)
		, m_PageSize(VirtualMemory::AlignToPageSize(page_size))
		, m_RegionIndex(0)
		, m_NextPageAddress(m_MemoryHeap.GetRegion(0)->Begin())
	{
	}

	HeapAllocator::HeapAllocator(VirtualMemoryHeap&& heap, size_t page_size)
		: m_MemoryHeap(std::move(heap))
		, m_PageSize(VirtualMemory::AlignToPageSize(page_size))
		, m_RegionIndex(m_MemoryHeap.Regions().size() - 1)
		, m_NextPageAddress(m_MemoryHeap.GetRegion(m_RegionIndex)->Begin())			// By default, assume that the next page address is the start of the region.
	{
		auto MemoryInformation = VirtualMemory::GetMemoryInformation(m_MemoryHeap.GetRegion(m_RegionIndex)->Begin(), m_MemoryHeap.GetRegionSize());

		if (MemoryInformation.m_State == VirtualMemory::State::kCommitted)			// Check if there is committed memory in the range. If that's the case, reassign the next page address.
		{
			m_NextPageAddress = MemoryInformation.m_Range.End();
		}
	}

	HeapAllocator::HeapAllocator(HeapAllocator&& other)
		: m_MemoryHeap(std::move(other.m_MemoryHeap))
		, m_PageSize(other.m_PageSize)
		, m_RegionIndex(other.m_RegionIndex)
		, m_NextPageAddress(other.m_NextPageAddress)
	{
		other.m_PageSize = 0;
		other.m_RegionIndex = 0;
		other.m_NextPageAddress = nullptr;
	}

	HeapAllocator::~HeapAllocator()
	{
	}

	void* HeapAllocator::AllocatePage()
	{
		_ASSERT(m_PageSize > 0);

		auto Region = m_MemoryHeap.GetRegion(m_RegionIndex);
		
		_ASSERT(Region);

		auto RegionEndAddress = Region->End();

		if(m_NextPageAddress == RegionEndAddress)								// Space is exhausted, reserve a new region
		{
			Region = m_MemoryHeap.AllocateRegion();

			m_NextPageAddress = Region->Begin();

			m_RegionIndex++;
		}

		auto PageAddress = m_NextPageAddress;

		m_NextPageAddress = Memory::AddOffset(m_NextPageAddress, m_PageSize);

		ZN_LOG(LogHeapAllocator, ELogVerbosity::Log, "Committing %i bytes on region %i. %i bytes left. \t Total committed memory: %i bytes. \t Region committed memory %i bytes"
			, m_PageSize
			, m_RegionIndex
			, Memory::GetDistance(Region->End(), m_NextPageAddress)
			, GetAllocatedMemory()
			, Memory::GetDistance(m_NextPageAddress, Region->Begin()));
		
		ZN_VM_CHECK(VirtualMemory::Commit(PageAddress, m_PageSize));

		MemoryDebug::MarkUninitialized(PageAddress, m_NextPageAddress);

		return PageAddress;
	}

	bool HeapAllocator::Free(void* address)
	{
		size_t RegionIndex = 0;
		
		if (m_MemoryHeap.GetRegionIndex(RegionIndex, address))
		{
			auto Region = m_MemoryHeap.GetRegion(RegionIndex);

			if (Region->Begin() != address)
			{
				ZN_LOG(LogHeapAllocator, ELogVerbosity::Error, "Cannot free address %p because it's not a region address.", address);

				return false;
			}

			return m_MemoryHeap.FreeRegion(RegionIndex);
		}
		else
		{
			ZN_LOG(LogHeapAllocator, ELogVerbosity::Error, "Trying to free address %p, but it has not been allocated with this allocator.", address);
		}

		return false;
	}

	size_t HeapAllocator::GetAllocatedMemory() const
	{
		return (m_MemoryHeap.GetRegionSize() * (m_MemoryHeap.Regions().size() - 1) + Memory::GetDistance(m_NextPageAddress, m_MemoryHeap.GetRegion(m_RegionIndex)->Begin()));	// Region Size * (Regions Num - 1) + (Next Page Address - Region Start Address)
	}

	bool HeapAllocator::IsAllocated(void* address) const
	{
		size_t RegionIndex = 0;

		if (m_MemoryHeap.GetRegionIndex(RegionIndex, address))
		{
			auto LastAllocatedAddress = RegionIndex == m_RegionIndex ? m_NextPageAddress : m_MemoryHeap.GetRegion(RegionIndex)->End();	// If is different, the memory should all committed.

			return Memory::GetDistance(address, LastAllocatedAddress) < 0;
		}

		return false;
	}
}
