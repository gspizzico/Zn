#include <Znpch.h>
#include "Core/Memory/Allocators/Strategies/DirectAllocationStrategy.h"
#include "Core/Memory/VirtualMemory.h"

namespace Zn
{
DirectAllocationStrategy::DirectAllocationStrategy(size_t min_allocation_size)
    : m_MinAllocationSize(VirtualMemory::AlignToPageSize(min_allocation_size))
{
}

void* DirectAllocationStrategy::Allocate(size_t size, size_t alignment)
{
    if (size < m_MinAllocationSize)
    {
        return nullptr;
    }

    size_t AllocationSize = VirtualMemory::AlignToPageSize(size);

    _ASSERT(AllocationSize < Memory::GetMemoryStatus().m_AvailPhys);

    auto Address = VirtualMemory::Allocate(AllocationSize);

    m_Allocations.emplace(Address);

    MemoryDebug::MarkUninitialized(Address, Memory::AddOffset(Address, AllocationSize));

    return Address;
}

bool DirectAllocationStrategy::Free(void* address)
{
    auto AllocationAddress = address;

    auto Removed = m_Allocations.erase(AllocationAddress);

    if (Removed != 0)
    {
        VirtualMemory::Release(AllocationAddress);
        return true;
    }
    else
    {
        return false;
    }
}
} // namespace Zn
