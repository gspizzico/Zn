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
        auto PageSize = GetPageSize();
        return glm::ceil<size_t>((size + PageSize - 1) / PageSize) * PageSize;
    }
}