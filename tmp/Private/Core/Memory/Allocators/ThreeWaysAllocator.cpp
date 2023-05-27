#include <Znpch.h>
#include <Core/Memory/Allocators/ThreeWaysAllocator.h>

using namespace Zn;

constexpr size_t kSmallAllocatorVM  = 2ull * (size_t) (StorageUnit::GigaByte);
constexpr size_t kMediumAllocatorVM = 128ull * (size_t) (StorageUnit::GigaByte);

constexpr size_t kLargeAllocatorPageSize = 64ull * (size_t) (StorageUnit::KiloByte);

ThreeWaysAllocator::ThreeWaysAllocator()
    : BaseAllocator()
    , region(kSmallAllocatorVM + kMediumAllocatorVM)
    , m_Small(MemoryRange(region.Begin(), kSmallAllocatorVM))
    , m_Medium(MemoryRange(Memory::AddOffset(region.Begin(), kSmallAllocatorVM), kMediumAllocatorVM))
    , m_Large(VirtualMemory::AlignToPageSize(kLargeAllocatorPageSize))
{
}

void* ThreeWaysAllocator::Malloc(size_t size, size_t alignment /*= DEFAULT_ALIGNMENT*/)
{
    if (size <= m_Small.GetMaxAllocationSize())
    {
        return m_Small.Allocate(size, alignment);
    }
    else if (size < m_Medium.MaxAllocationSize())
    {
        return m_Medium.Allocate(size, alignment);
    }
    else
    {
        return m_Large.Allocate(size, alignment);
    }

    return nullptr;
}

bool ThreeWaysAllocator::Free(void* ptr)
{
    bool success = false;

    if (ptr)
    {
        success = m_Small.Free(ptr);

        if (!success)
        {
            success = m_Medium.Free(ptr);
        }

        if (!success)
        {
            success = m_Large.Free(ptr);
        }
    }

    return success;
}