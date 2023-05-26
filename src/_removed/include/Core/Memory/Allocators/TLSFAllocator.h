#pragma once
#include <array>
#include "Core/Memory/Allocators/PageAllocator.h"
#include "Core/Memory/VirtualMemory.h"
#include "Core/Math/Math.h"
#include <Core/HAL/PlatformTypes.h>

namespace Zn
{

class TLSFAllocator
{
  public:
    static constexpr size_t kJ = 3;

    static constexpr size_t kNumberOfPools = 10;

    static constexpr size_t kNumberOfLists = 1 << kJ; // pow(2, kJ)

    static constexpr size_t kStartFl = 8; // log2(kMinBlockSize)

    static constexpr size_t kMaxAllocationSize = (1 << (kStartFl + kNumberOfPools - 2)); // 128k is block size, 64k is max allocation size.

    static constexpr size_t kBlockSize = kMaxAllocationSize * 2;

    struct FreeBlock
    {
      public:
        static constexpr size_t kMinBlockSize = 256;

        static FreeBlock* New(const MemoryRange& block_range);

        FreeBlock(size_t blockSize, FreeBlock* const previous, FreeBlock* const next);

        ~FreeBlock();

        size_t Size() const
        {
            return m_BlockSize;
        }

#if ZN_DEBUG
        void LogDebugInfo(bool recursive) const;
#endif

        FreeBlock* m_Previous = nullptr;
        size_t     m_BlockSize;
        u8         m_Flags        = 0;
        FreeBlock* m_PreviousFree = nullptr;
        FreeBlock* m_NextFree     = nullptr;

        enum Flags
        {
            kFreeBit     = 1,
            kPrevFreeBit = 1 << 1
        };

      private:
        static constexpr bool kMarkFreeOnDelete = false;
    };

    TLSFAllocator(MemoryRange inMemoryRange);

    __declspec(allocator) void* Allocate(size_t size, size_t alignment = 1);

    bool Free(void* address);

    size_t GetAllocatedMemory() const
    {
        return m_Memory.GetUsedMemory();
    }

    static constexpr size_t MinAllocationSize()
    {
        return FreeBlock::kMinBlockSize;
    }

    static constexpr size_t MaxAllocationSize()
    {
        return kMaxAllocationSize;
    }

#if ZN_DEBUG
    void LogDebugInfo() const;

    void Verify() const;

    void VerifyWrite(MemoryRange range) const;
#endif

  private:
    using index_type = unsigned long;

    using FreeListMatrix = FreeBlock* [kNumberOfPools][kNumberOfLists];

    bool MappingInsert(size_t size, index_type& o_fl, index_type& o_sl);

    bool MappingSearch(size_t size, index_type& o_fl, index_type& o_sl);

    bool FindSuitableBlock(index_type& o_fl, index_type& o_sl);

    FreeBlock* MergePrevious(FreeBlock* block);

    FreeBlock* MergeNext(FreeBlock* block);

    void RemoveBlock(FreeBlock* block);

    void AddBlock(FreeBlock* block);

    bool Decommit(FreeBlock* block);

    PageAllocator m_Memory;

    FreeListMatrix m_FreeLists;

    uint16_t m_FL;

    uint16_t m_SL[kNumberOfPools];

    CriticalSection criticalSection;
};
} // namespace Zn
