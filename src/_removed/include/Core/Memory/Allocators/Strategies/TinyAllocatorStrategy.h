#pragma once

#include <Core/Memory/Allocators/PageAllocator.h>
#include <Core/Containers/Vector.h>
#include <Core/HAL/PlatformTypes.h>

#include <array>

namespace Zn
{
class TinyAllocatorStrategy
{
  public:
    TinyAllocatorStrategy(MemoryRange inMemoryRange);

    void* Allocate(size_t size, size_t alignment = sizeof(void*));

    bool Free(void* address);

    size_t GetMaxAllocationSize() const;

  private:
    struct FreeBlock
    {
        FreeBlock* m_Next      = nullptr;
        size_t     m_FreeSlots = 0;
    };

    size_t GetFreeListIndex(size_t size) const;

    size_t GetFreeListIndex(void* address) const;

    size_t GetSlotSize(size_t freeListIndex) const;

    static constexpr auto kFreeBlockSize = sizeof(FreeBlock);

    PageAllocator m_Memory;

    std::array<FreeBlock*, 16> m_FreeLists;
    std::array<size_t, 16>     m_NumAllocations;

    size_t m_NumFreePages;

    CriticalSection criticalSection;

    static constexpr size_t kMaxAllocationSize = 16 * 16;
};
} // namespace Zn
