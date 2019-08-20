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
		: m_Address(VirtualMemory::Reserve(VirtualMemory::AlignToPageSize(capacity)))
		, m_Range(m_Address, Memory::Align(capacity, VirtualMemory::GetPageSize()))
	{
	}

	VirtualMemoryRegion::VirtualMemoryRegion(VirtualMemoryRegion&& other) noexcept
		: m_Address(other.m_Address)
		, m_Range(std::move(other.m_Range))
	{
		other.m_Address = nullptr;
	}

	VirtualMemoryRegion::~VirtualMemoryRegion()
	{
		if (m_Address)
		{
			VirtualMemory::Release(m_Address);
		}
	}
	
	VirtualMemoryHeap::VirtualMemoryHeap(size_t region_size)
		: m_RegionSize(VirtualMemory::AlignToPageSize(region_size))
		, m_Regions({ std::make_shared<VirtualMemoryRegion>(m_RegionSize) })
		//, m_FreeRegions()
	{
	}

	VirtualMemoryHeap::VirtualMemoryHeap(VirtualMemoryHeap&& other) noexcept
	{
		m_RegionSize = other.m_RegionSize;
		m_Regions = std::move(other.m_Regions);
		//m_FreeRegions = std::move(other.m_FreeRegions);
	}

	bool VirtualMemoryHeap::IsValidAddress(void* address) const
	{
		auto Predicate = [address_ = address](const SharedPtr<VirtualMemoryRegion>& Region) { return Region->Range().Contains(address_); };

		return std::any_of(m_Regions.cbegin(), m_Regions.cend(), Predicate);
	}

	void* VirtualMemoryHeap::AllocateRegion()
	{
		return **m_Regions.emplace_back(std::make_shared<VirtualMemoryRegion>(VirtualMemoryRegion(m_RegionSize)));
		
		//m_FreeRegions.emplace_back(m_Regions.size() - 1);
	}

	bool VirtualMemoryHeap::FreeRegion(size_t region_index)
	{
		auto IsValidIndex = [this](size_t index) { return index >= 0 && index < m_Regions.size(); };
		//auto IsFreeBlock = [this](size_t index) { return std::find(m_Regions.cbegin(), m_Regions.cend(), index) != m_Regions.cend(); };
		
		if (IsValidIndex(region_index)) //&& IsFreeBlock(region_index))
		{	
			auto MemoryInformation = VirtualMemory::GetMemoryInformation(**m_Regions[region_index], m_RegionSize);

			if (MemoryInformation.m_State > VirtualMemory::State::kReserved)
			{
				ZN_LOG(LogMemory, ELogVerbosity::Error, "VirtualMemoryHeap::FreeRegion has failed. Trying to free a region that is committed or freed.");
				return false;
			}
			
			m_Regions.erase(m_Regions.begin() + region_index);

			return true;
		}

		return false;
	}
}