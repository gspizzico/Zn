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
		_ASSERT(Memory::GetMemoryStatus().m_AvailPhys >= size);

		if (Memory::GetMemoryStatus().m_AvailPhys < size)
		{
			abort(); // OOM
		}
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

	VirtualMemoryInformation VirtualMemory::GetMemoryInformation(MemoryRange range)
	{
		return VirtualMemory::GetMemoryInformation(range.Begin(), range.Size());
	}

	VirtualMemoryRegion::VirtualMemoryRegion(size_t capacity)
		: m_Range(VirtualMemory::Reserve(VirtualMemory::AlignToPageSize(capacity)), Memory::Align(capacity, VirtualMemory::GetPageSize()))
	{
	}

	VirtualMemoryRegion::VirtualMemoryRegion(VirtualMemoryRegion&& other) noexcept
		: m_Range(std::move(other.m_Range))
	{
	}

	VirtualMemoryRegion::VirtualMemoryRegion(MemoryRange range)
		: m_Range(range)
	{
		_ASSERT(VirtualMemory::GetMemoryInformation(m_Range).m_State == VirtualMemory::State::kReserved);
	}

	VirtualMemoryRegion::~VirtualMemoryRegion()
	{
		if (auto BaseAddress = m_Range.Begin())
		{
			VirtualMemory::Release(BaseAddress);
		}
	}
}