#include "Core/Memory/VirtualMemory.h"
#include "Core/HAL/PlatformTypes.h"
#include "glm.hpp"

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

	MemoryResource::MemoryResource(size_t capacity, size_t alignment)
		: m_Resource(VirtualMemory::Reserve(Memory::Align(capacity, alignment)))
		, m_Range(m_Resource, Memory::Align(capacity, alignment))
	{
	}

	MemoryResource::MemoryResource(MemoryResource&& other) noexcept
		: m_Resource(other.m_Resource)
		, m_Range(std::move(other.m_Range))
	{
		other.m_Resource = nullptr;
	}

	MemoryResource::~MemoryResource()
	{
		if (m_Resource)
		{
			VirtualMemory::Release(m_Resource);
		}
	}
}