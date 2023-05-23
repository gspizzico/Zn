#include <Znpch.h>
#include <Core/Memory/Allocators/Strategies/TinyAllocatorStrategy.h>
#include <Core/Memory/Memory.h>

using namespace Zn;

TinyAllocatorStrategy::TinyAllocatorStrategy(MemoryRange inMemoryRange)
    : m_Memory(inMemoryRange, VirtualMemory::GetPageSize())
    , m_FreeLists()
    , m_NumAllocations()
    , m_NumFreePages(0)
{
    std::fill(m_FreeLists.begin(), m_FreeLists.end(), nullptr);

    double PageSize = static_cast<double>(m_Memory.PageSize());

    for (auto index = 0; index < m_NumAllocations.size(); ++index)
    {
        m_NumAllocations[index] = static_cast<size_t>(std::floorl(PageSize / GetSlotSize(index))) - 1u;
    }
}

void* TinyAllocatorStrategy::Allocate(size_t size, size_t alignment)
{
    // #todo handle alignment

    _ASSERT(size <= kMaxAllocationSize);

    size_t FreeListIndex = GetFreeListIndex(size);

    const size_t SlotSize = 16 * (FreeListIndex + 1);

    criticalSection.Lock();

    auto& CurrentFreeList = m_FreeLists[FreeListIndex];

    // If we don't have any page in the free list, request a new one.
    if (CurrentFreeList == nullptr)
    {
        void* PageAddress = m_Memory.Allocate();

        size_t* PageHeader = reinterpret_cast<size_t*>(PageAddress);

        *PageHeader = FreeListIndex;

        FreeBlock* Slot = new (Memory::AddOffset(PageAddress, SlotSize)) FreeBlock();

        Slot->m_Next      = nullptr;
        Slot->m_FreeSlots = m_NumAllocations[FreeListIndex];

        CurrentFreeList = Slot;
    }

    void* Allocation = static_cast<void*>(CurrentFreeList);

    FreeBlock Slot = *CurrentFreeList;

    if (Slot.m_Next != nullptr)
    {
        CurrentFreeList = Slot.m_Next;

        if (CurrentFreeList->m_Next == nullptr)
        {
            m_NumFreePages--;
        }
    }
    else
    {
        if (Slot.m_FreeSlots == 1)
        {
            CurrentFreeList = nullptr;
        }
        else
        {
            CurrentFreeList = new (Memory::AddOffset(Allocation, SlotSize)) FreeBlock();

            CurrentFreeList->m_Next      = nullptr;
            CurrentFreeList->m_FreeSlots = Slot.m_FreeSlots - 1;
        }
    }

    MemoryDebug::MarkUninitialized(Allocation, Memory::AddOffset(Allocation, SlotSize));

    _ASSERT(CurrentFreeList == nullptr || reinterpret_cast<uintptr_t>(CurrentFreeList->m_Next) < (uintptr_t) 0x00007FF000000000);

    criticalSection.Unlock();

    return Allocation;
}

bool TinyAllocatorStrategy::Free(void* address)
{
    if (!m_Memory.IsAllocated(address))
    {
        return false;
    }

    size_t FreeListIndex = GetFreeListIndex(address);

    const size_t SlotSize = GetSlotSize(FreeListIndex);

    criticalSection.Lock();

    auto& CurrentFreeList = m_FreeLists[FreeListIndex];

    MemoryDebug::MarkFree(address, Memory::AddOffset(address, SlotSize));

    if (CurrentFreeList == nullptr)
    {
        CurrentFreeList = new (address) FreeBlock();

        CurrentFreeList->m_Next      = nullptr;
        CurrentFreeList->m_FreeSlots = 1;
    }
    else
    {
        FreeBlock* Next              = CurrentFreeList;
        CurrentFreeList              = new (address) FreeBlock();
        CurrentFreeList->m_Next      = Next;
        CurrentFreeList->m_FreeSlots = 0;

        if (Next->m_FreeSlots > 0)
        {
            m_NumFreePages++;
        }
    }

    _ASSERT(CurrentFreeList == nullptr || reinterpret_cast<uintptr_t>(CurrentFreeList->m_Next) < (uintptr_t) 0x00007FF000000000);

    criticalSection.Unlock();

    return true;
}

size_t TinyAllocatorStrategy::GetMaxAllocationSize() const
{
    return 255;
}

size_t TinyAllocatorStrategy::GetFreeListIndex(size_t size) const
{
    return std::max(int(size >> 4), 0);
}

size_t Zn::TinyAllocatorStrategy::GetFreeListIndex(void* address) const
{
    void* PageAddress = m_Memory.GetPageAddress(address);

    size_t FreeListIndex = *reinterpret_cast<size_t*>(PageAddress);

    _ASSERT(FreeListIndex >= 0 && FreeListIndex < 16);

    return FreeListIndex;
}

size_t TinyAllocatorStrategy::GetSlotSize(size_t freeListIndex) const
{
    return 16 * (freeListIndex + 1);
}
