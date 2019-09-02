#include "Core/Memory/VirtualMemory.h"
#include "Core/HAL/PlatformTypes.h"
#include "Core/Log/LogMacros.h"
#include <algorithm>

DEFINE_LOG_CATEGORY(LogMemory, Zn::ELogVerbosity::Warning)

namespace Zn
{
    void* VirtualMemory::Reserve(size_t size)
    {
        return PlatformVirtualMemory::Reserve(size);
    }
    void* VirtualMemory::Allocate(size_t size)
    {
        return PlatformVirtualMemory::Allocate(size);
    }
    bool VirtualMemory::Release(void * address)
    {
        return PlatformVirtualMemory::Release(address);
    }
    bool VirtualMemory::Commit(void * address, size_t size)
    {
        return PlatformVirtualMemory::Commit(address, size);
    }
    bool VirtualMemory::Decommit(void * address, size_t size)
    {
        return PlatformVirtualMemory::Decommit(address, size);
    }
    size_t VirtualMemory::GetPageSize()
    {
        return PlatformVirtualMemory::GetPageSize();
    }
    size_t VirtualMemory::AlignToPageSize(size_t size)
    {
		return Memory::Align(size, GetPageSize());
    }

	VirtualMemoryInformation VirtualMemory::GetMemoryInformation(void* address, size_t size)
	{
		return PlatformVirtualMemory::GetMemoryInformation(address, size);
	}

	VirtualMemoryRegion::VirtualMemoryRegion(size_t capacity)
		: m_Range(VirtualMemory::Reserve(VirtualMemory::AlignToPageSize(capacity)), Memory::Align(capacity, VirtualMemory::GetPageSize()))
	{
	}

	VirtualMemoryRegion::VirtualMemoryRegion(VirtualMemoryRegion&& other) noexcept
		: m_Range(std::move(other.m_Range))
	{
	}

	VirtualMemoryRegion::~VirtualMemoryRegion()
	{
		if (auto BaseAddress = m_Range.Begin())
		{
			VirtualMemory::Release(BaseAddress);
		}
	}
	
	VirtualMemoryHeap::VirtualMemoryHeap(size_t region_size)
		: m_RegionSize(VirtualMemory::AlignToPageSize(region_size))
		, m_Regions({ std::make_shared<VirtualMemoryRegion>(m_RegionSize) })
	{
	}

	VirtualMemoryHeap::VirtualMemoryHeap(VirtualMemoryHeap&& other) noexcept
		: m_RegionSize(other.m_RegionSize)
		, m_Regions(std::move(other.m_Regions))
	{
		other.m_RegionSize = 0;
	}

	bool VirtualMemoryHeap::IsValidAddress(void* address) const
	{
		auto Predicate = [address_ = address](const SharedPtr<VirtualMemoryRegion>& Region) { return Region->Range().Contains(address_); };

		return std::any_of(m_Regions.cbegin(), m_Regions.cend(), Predicate);
	}

	SharedPtr<VirtualMemoryRegion> VirtualMemoryHeap::AllocateRegion()
	{
		return m_Regions.emplace_back(std::make_shared<VirtualMemoryRegion>(VirtualMemoryRegion(m_RegionSize)));
	}

	bool VirtualMemoryHeap::FreeRegion(size_t region_index)
	{	
		if (IsValidIndex(region_index))
		{	
			auto MemoryInformation = VirtualMemory::GetMemoryInformation(GetRegion(region_index)->Begin(), m_RegionSize);

			if (MemoryInformation.m_State > VirtualMemory::State::kReserved)
			{
				ZN_LOG(LogMemory, ELogVerbosity::Error, "VirtualMemoryHeap::FreeRegion has failed. Trying to free a region that is committed or already freed.");

				return false;
			}
			
			m_Regions.erase(m_Regions.begin() + region_index);

			return true;
		}

		return false;
	}

	bool VirtualMemoryHeap::GetRegionIndex(size_t& out_region_index, void* address) const
	{
		auto Predicate = [address_ = address](const SharedPtr<VirtualMemoryRegion>& Region) { return Region->Range().Contains(address_); };

		auto It = std::find_if(m_Regions.cbegin(), m_Regions.cend(), Predicate);

		const bool IsValidIterator = It != m_Regions.cend();
		
		if(IsValidIterator)
		{
			out_region_index = std::distance(m_Regions.begin(), It);
		}

		return IsValidIterator;
	}
	VirtualMemoryPage::VirtualMemoryPage(MemoryRange range)
		: m_Range(range)
		, m_AllocatedSize(0)
	{
	}
	VirtualMemoryPage::VirtualMemoryPage(VirtualMemoryPage&& other) noexcept
		: m_Range(std::move(other.m_Range))
		, m_AllocatedSize(other.m_AllocatedSize)
	{
		other.m_AllocatedSize = 0;
	}
	
	void VirtualMemoryPage::TrackAllocation(size_t size)
	{
		m_AllocatedSize += size;
		_ASSERT(m_AllocatedSize <= Size());
	}
	void VirtualMemoryPage::TrackFree(size_t size)
	{
		m_AllocatedSize -= size;
		_ASSERT(m_AllocatedSize <= Size());
	}
}