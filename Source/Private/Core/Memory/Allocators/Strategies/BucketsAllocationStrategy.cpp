#include <Znpch.h>
#include "Core/Memory/Allocators/Strategies/BucketsAllocationStrategy.h"

DEFINE_STATIC_LOG_CATEGORY(LogBucketsAllocationStrategy, ELogVerbosity::Log);

namespace Zn
{
BucketsAllocationStrategy::BucketsAllocationStrategy(SharedPtr<PageAllocator> memory, size_t max_allocation_size, size_t allocation_step /*=sizeof(uintptr_t)*/)
    : m_Memory(memory)
    , m_Buckets()
{
    _ASSERT(allocation_step >= kMinAllocationSize);
    _ASSERT(max_allocation_size >= kMinAllocationSize);

    m_AllocationStep         = Memory::Align(allocation_step, sizeof(uintptr_t));
    size_t MaxAllocationSize = Memory::Align(max_allocation_size, m_AllocationStep);

    const size_t AllocatorsCount = size_t(MaxAllocationSize / m_AllocationStep);
    m_Buckets.reserve(AllocatorsCount);

    for (size_t Index = 0; Index < AllocatorsCount; ++Index)
    {
        m_Buckets.emplace_back(FixedSizeAllocator(m_AllocationStep * (Index + 1), m_Memory));
    }
}

void* BucketsAllocationStrategy::Allocate(size_t size, size_t alignment)
{
    const size_t InternalAlignment = Memory::Align(alignment, m_AllocationStep); // Always alignt to at least 8bytes

    const size_t AllocationSize = Memory::Align(size, InternalAlignment);

    _ASSERT(AllocationSize <= GetMaxAllocationSize()); // Ensure that we can allocate this size.

    size_t AllocatorIndex = (AllocationSize / m_AllocationStep) - 1;

    _ASSERT(AllocatorIndex >= 0 && AllocatorIndex < m_Buckets.size());

    return m_Buckets[AllocatorIndex].Allocate();
}

void BucketsAllocationStrategy::Free(void* address)
{
    _ASSERT(m_Memory->Range().Contains(address));

    auto PageAddress = FixedSizeAllocator::FSAPage::GetPageFromAnyAddress(address, m_Memory->Range().Begin(), m_Memory->PageSize());

    const size_t& AllocationSize = PageAddress->m_AllocationSize;

    size_t AllocatorIndex = (AllocationSize / m_AllocationStep) - 1;

    _ASSERT(AllocatorIndex >= 0 && AllocatorIndex < m_Buckets.size());

    m_Buckets[AllocatorIndex].Free(address);
}

size_t BucketsAllocationStrategy::GetMaxAllocationSize() const
{
    return m_Buckets.size() * m_AllocationStep;
}

size_t BucketsAllocationStrategy::GetWastedMemory() const
{
    size_t SumAllUsedMemory = 0;

    for (size_t Index = 0; Index < m_Buckets.size(); ++Index)
    {
        const auto& Allocator = m_Buckets[Index];

        const auto& FreePageList = Allocator.GetFreePageList();

        const size_t CommittedFreePagesTotalSize = FreePageList.size() * VirtualMemory::GetPageSize();

        if (CommittedFreePagesTotalSize > 0)
        {
            size_t SumUsedMemory = 0;

            for (const auto& Page : Allocator.GetFreePageList())
            {
                auto pPage = reinterpret_cast<FixedSizeAllocator::FSAPage*>(Page);

                SumUsedMemory += pPage->GetAllocatedMemory();
            }

            ZN_LOG(
                LogBucketsAllocationStrategy, ELogVerbosity::Verbose, "Allocator %p \t AllocationSize %i \t Usage: %.2f", &Allocator,
                (Index + 1) * m_AllocationStep, float(SumUsedMemory) / float(CommittedFreePagesTotalSize));

            SumAllUsedMemory += SumUsedMemory;
        }
    }

    return m_Memory->GetUsedMemory() - SumAllUsedMemory;
}
} // namespace Zn