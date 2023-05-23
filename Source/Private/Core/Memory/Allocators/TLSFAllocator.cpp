#include <Znpch.h>
#include "Core/Memory/Allocators/TLSFAllocator.h"
#include "Core/Memory/Memory.h"

DEFINE_STATIC_LOG_CATEGORY(LogTLSF_Allocator, ELogVerbosity::Log);

#define TLSF_ENABLE_DECOMMIT 1

#pragma push_macro("ZN_LOG")
#undef ZN_LOG

#define ZN_LOG(...)

namespace Zn
{
using FreeBlock = TLSFAllocator::FreeBlock;

static constexpr sizet kFreeBlockOverhead = offsetof(FreeBlock, m_Flags) + sizeof(FreeBlock::m_Flags);

//	===	freeBlock ===

FreeBlock* TLSFAllocator::FreeBlock::New(const MemoryRange& block_range)
{
    MemoryDebug::MarkFree(block_range.Begin(), block_range.End());

    return new (block_range.Begin()) FreeBlock(block_range.Size(), nullptr, nullptr);
}

FreeBlock::FreeBlock(size_t blockSize, FreeBlock* const previous, FreeBlock* const next)
    : m_BlockSize(blockSize)
    , m_Flags(kFreeBit)
    , m_PreviousFree(previous)
    , m_NextFree(next)
{
    _ASSERT(blockSize >= kMinBlockSize);

    // new (GetFooter()) freeBlock::Footer {m_BlockSize, previous, next};
}

FreeBlock::~FreeBlock()
{
    if constexpr (kMarkFreeOnDelete)
    {
        Memory::MarkMemory(this, Memory::AddOffset(this, m_BlockSize), 0);
    }
}

#if ZN_DEBUG
void FreeBlock::LogDebugInfo(bool recursive) const
{
    ZN_LOG(LogTLSF_Allocator, ELogVerbosity::Log, "BlockSize %i \t Address %p", m_BlockSize, this);

    if ((m_Flags & Flags::kFreeBit) > 0 && m_NextFree != nullptr)
    {
        m_NextFree->LogDebugInfo(recursive);
    }
}
#endif

//	===	TLSFAllocator ===

TLSFAllocator::TLSFAllocator(MemoryRange inMemoryRange)
    : m_Memory(inMemoryRange, kBlockSize)
    , m_FreeLists()
    , m_FL(0)
    , m_SL()
{
    _ASSERT(inMemoryRange.Size() > kMaxAllocationSize);
    memset(const_cast<u16*>(ArrayData(m_SL)), 0, ArrayLength(m_SL));
}

void* TLSFAllocator::Allocate(size_t size, size_t alignment)
{
    // criticalSection.Lock();

    size_t AllocationSize =
        Memory::Align(size + kFreeBlockOverhead, FreeBlock::kMinBlockSize); // FREEBLOCK -> {[BLOCK_SIZE][STORAGE|FOOTER]}

    index_type fl = 0, sl = 0;
    MappingSearch(AllocationSize, fl, sl);

    FreeBlock* freeBlock = nullptr;

    size_t BlockSize = 0;

    if (!FindSuitableBlock(fl, sl))
    {
        BlockSize = m_Memory.PageSize();

        void* AllocatedMemory = m_Memory.Allocate(); // Allocate a new page if there is no available block for the requested size.

        freeBlock = FreeBlock::New({AllocatedMemory, BlockSize});
    }
    else
    {
        freeBlock = m_FreeLists[fl][sl]; // Pop the block from the list.

        RemoveBlock(freeBlock);

        BlockSize = freeBlock->Size();
    }

    _ASSERT(BlockSize >= AllocationSize);

    // If the free block is bigger than the allocation size, try to create a new smaller block from it.
    // TODO: Consider header size
    if (auto NewBlockSize = BlockSize - AllocationSize; NewBlockSize >= (FreeBlock::kMinBlockSize))
    {
        BlockSize = AllocationSize;

        auto NewBlockAddress = Memory::AddOffset(freeBlock, BlockSize); // BIG FREEBLOCK -> {[REQUESTED_BLOCK][NEW_BLOCK]}

        TLSFAllocator::FreeBlock* NewBlock = FreeBlock::New({NewBlockAddress, NewBlockSize});

        AddBlock(NewBlock);

        NewBlock->m_Previous = freeBlock;
    }

    freeBlock->m_BlockSize = BlockSize; // Write at the beginning of the block its size in order to safely free the memory when requested.

    freeBlock->m_Flags = freeBlock->m_Flags & ~FreeBlock::kFreeBit;

    MemoryDebug::TrackAllocation(freeBlock, BlockSize);

    ZN_LOG(LogTLSF_Allocator, ELogVerbosity::Verbose, "Allocate \t %p, Size: %i", freeBlock, BlockSize);

    auto AllocationRange = MemoryRange(Memory::AddOffset(freeBlock, kFreeBlockOverhead), BlockSize - kFreeBlockOverhead);

    _ASSERT(AllocationRange.Size() >= size);

    return AllocationRange.Begin();
}

bool TLSFAllocator::Free(void* address)
{
#if TLSF_ENABLE_DECOMMIT
    if (!m_Memory.IsAllocated(address))
#else
    if (!m_Memory.IsAllocatedFast(address))
#endif
    {
        return false;
    }

    FreeBlock* BlockAddress = reinterpret_cast<FreeBlock*>(Memory::SubOffset(address, kFreeBlockOverhead)); // Recover this block size

    _ASSERT((BlockAddress->m_Flags & FreeBlock::kFreeBit) != FreeBlock::kFreeBit);

    uintptr_t BlockSize = BlockAddress->m_BlockSize;

    ZN_LOG(LogTLSF_Allocator, ELogVerbosity::Verbose, "Free\t %p, Size: %i", BlockAddress, BlockSize);

    BlockAddress->m_Flags |= FreeBlock::kFreeBit;

    FreeBlock* NewBlock = BlockAddress;

    MemoryDebug::TrackDeallocation(BlockAddress);

    NewBlock = MergePrevious(NewBlock); // Try merge the previous physical block

    NewBlock = MergeNext(NewBlock); // Try merge the next physical block

    if (!Decommit(NewBlock))
    {
        AddBlock(NewBlock);
    }

    return true;
}

#if ZN_DEBUG

void TLSFAllocator::LogDebugInfo() const
{
    for (int fl = 0; fl < kNumberOfPools; ++fl)
    {
        for (int sl = 0; sl < kNumberOfLists; ++sl)
        {
            ZN_LOG(LogTLSF_Allocator, ELogVerbosity::Log, "FL \t %i \t SL \t %i", fl, sl);

            if (auto FreeBlock = m_FreeLists[fl][sl])
            {
                FreeBlock->LogDebugInfo(true);
            }
        }
    }
}

#endif

bool TLSFAllocator::MappingInsert(size_t size, index_type& o_fl, index_type& o_sl)
{
    if (size <= kMaxAllocationSize)
    {
        _BitScanReverse64(&o_fl, size); // FLS -> Find most significant bit and returns log2 of it -> (2^o_fl);

        o_sl = size < FreeBlock::kMinBlockSize ? 0ul : static_cast<index_type>((size >> (o_fl - kJ)) - kNumberOfLists);

        o_fl -= kStartFl; // Remove kStartFl because we are not starting from lists of size 2.
    }
    else
    {
        o_fl = kNumberOfPools - 1;
        o_sl = kNumberOfLists - 1;
    }

    ZN_LOG(LogTLSF_Allocator, ELogVerbosity::Verbose, "Size %u \t fl %u \t sl %u", size, o_fl, o_sl);

    return o_fl < kNumberOfPools && o_sl < kNumberOfLists;
}

bool TLSFAllocator::MappingSearch(size_t size, index_type& o_fl, index_type& o_sl)
{
    o_fl = 0;
    o_sl = 0;

    if (size >= FreeBlock::kMinBlockSize) // For blocks smaller than the minimum block, use always the minimum block
    {
        index_type fl = 0;

        _BitScanReverse64(&fl, size);

        size += (1ull << (fl - kJ)) -
                1; // Find a block bigger than the appropriate one, in order to correctly fit the obj to be allocated + its header.

        return MappingInsert(size, o_fl, o_sl);
    }

    return true; // This should always return 0, 0.
}

bool TLSFAllocator::FindSuitableBlock(index_type& o_fl, index_type& o_sl)
{
    // From Implementation of a constant-time dynamic storage allocator -> http://www.gii.upv.es/tlsf/files/spe_2008.pdf

    index_type fl = 0, sl = 0;

    u16 Bitmap = m_SL[o_fl] & (0xFFFFFFFFFFFFFFFF << o_sl);

    if (Bitmap != 0)
    {
        _BitScanForward64(&sl, Bitmap); // FFS -> Find least significant bit log2 (2^sl)

        fl = o_fl;
    }
    else
    {
        Bitmap = m_FL & (0xFFFFFFFFFFFFFFFF << (o_fl + 1));

        if (Bitmap == 0)
        {
            return false; // No available blocks have been found. Commit new pages or OOM.
        }

        _BitScanForward64(&fl, Bitmap);

        _BitScanForward64(&sl, m_SL[fl]);
    }

    o_fl = fl;

    o_sl = sl;

    return true;
}

TLSFAllocator::FreeBlock* TLSFAllocator::MergePrevious(FreeBlock* block)
{
    if ((block->m_Flags & FreeBlock::kPrevFreeBit) > 0)
    {
        FreeBlock* previous = block->m_Previous;
        if ((previous->m_Flags & FreeBlock::kFreeBit) > 0)
        {
            RemoveBlock(previous);

            sizet newSize         = previous->m_BlockSize + block->m_BlockSize;
            previous->m_BlockSize = newSize;

            _ASSERT(block->m_NextFree == nullptr);

            return previous;
        }
    }

    return block;
}

TLSFAllocator::FreeBlock* TLSFAllocator::MergeNext(FreeBlock* block)
{
    FreeBlock* next = static_cast<FreeBlock*>(Memory::AddOffset(block, block->m_BlockSize));

#if TLSF_ENABLE_DECOMMIT
    if (!m_Memory.IsAllocated(next))
#else
    if (!m_Memory.IsAllocatedFast(next))
#endif
    {
        if ((next->m_Flags & FreeBlock::kFreeBit) > 0)
        {
            sizet size = next->m_BlockSize;

            ZN_LOG(LogTLSF_Allocator, ELogVerbosity::Verbose, "MergeNext \t Block: %p \t Next: %p", block, next);

            RemoveBlock(next);

            block->m_BlockSize += next->m_BlockSize;
        }
    }
    return block;
}

void TLSFAllocator::RemoveBlock(FreeBlock* block)
{
    _ASSERT((block->m_Flags & FreeBlock::kFreeBit) > 0);

    index_type fl = 0, sl = 0;

    MappingInsert(block->Size(), fl, sl);

    ZN_LOG(LogTLSF_Allocator, ELogVerbosity::Verbose, "RemoveBlock \t Block: %p\t  Size: %i, fl %i, sl %i", block, block->Size(), fl, sl);

    if (m_FreeLists[fl][sl] == block)
    {
        FreeBlock* next     = block->m_NextFree;
        m_FreeLists[fl][sl] = next;

        if (next == nullptr) // Since we are replacing the head, make sure that if it's null we remove this block from FL/SL.
        {
            m_SL[fl] &= ~(1ull << sl);
            if (m_SL[fl] == 0)
            {
                m_FL &= ~(1ull << fl);
            }
        }
        else
        {
            next->m_PreviousFree = nullptr;
            next->m_Flags        = next->m_Flags & ~FreeBlock::kPrevFreeBit;
        }
    }
    else
    {
        ZN_LOG(LogTLSF_Allocator, ELogVerbosity::Verbose, "\t Previous -> %p", block->m_PreviousFree);
        ZN_LOG(LogTLSF_Allocator, ELogVerbosity::Verbose, "\t\t\t Previous.Next -> %p", block->m_PreviousFree->m_NextFree);

        _ASSERT(block->m_PreviousFree != nullptr);

        block->m_PreviousFree->m_NextFree = block->m_NextFree; // Remap previous block to next block

        if (block->m_NextFree)
        {
            _ASSERT(block->m_NextFree->m_PreviousFree != nullptr);

            block->m_NextFree->m_PreviousFree = block->m_PreviousFree;
        }
    }
}

void TLSFAllocator::AddBlock(FreeBlock* block)
{
    index_type fl = 0, sl = 0;

    MappingInsert(block->Size(), fl, sl);

    ZN_LOG(LogTLSF_Allocator, ELogVerbosity::Verbose, "AddBlock \t Block: %p\t  Size: %i, fl %i, sl %i", block, block->Size(), fl, sl);

    m_FL |= (1ull << fl);
    m_SL[fl] |= (1ull << sl);

    block->m_NextFree     = nullptr;
    block->m_PreviousFree = nullptr;

    if (auto Head = m_FreeLists[fl][sl]; Head != nullptr)
    {
        ZN_LOG(LogTLSF_Allocator, ELogVerbosity::Verbose, "\t Previous Head -> %p", Head);

        block->m_NextFree = Head;

        Head->m_PreviousFree = block;
        Head->m_Flags |= FreeBlock::kPrevFreeBit;

        ZN_LOG(LogTLSF_Allocator, ELogVerbosity::Verbose, "\t\t (Previous Head).Previous -> %p", Head->Previous());
    }

#if ENABLE_MEM_VERIFY
    VERIFY_WRITE_CALL(MemoryRange(block, block->Size()));
#endif

    block->m_Flags |= FreeBlock::kFreeBit;
    m_FreeLists[fl][sl] = block;

    ZN_LOG(LogTLSF_Allocator, ELogVerbosity::Verbose, "\t\t Next -> %p", block->Next());
}

bool TLSFAllocator::Decommit(FreeBlock* block)
{
#if TLSF_ENABLE_DECOMMIT
    const auto BlockSize = block->Size();

    if (BlockSize >= (m_Memory.PageSize() + (FreeBlock::kMinBlockSize * 2)))
    {
        auto SPA = Memory::Align(block, m_Memory.PageSize());

        auto NextPhysicalBlockAddress = Memory::AddOffset(block, BlockSize);

        auto SPA_Alignment = Memory::GetDistance(SPA, block);

        if (BlockSize - SPA_Alignment < m_Memory.PageSize())
        {
            return false;
        }

        auto EPA = Memory::AddOffset(SPA, m_Memory.PageSize());

        auto LastBlockSize = Memory::GetDistance(NextPhysicalBlockAddress, EPA);

        if (SPA_Alignment > 0 && SPA_Alignment < FreeBlock::kMinBlockSize)
            return false;
        if (LastBlockSize > 0 && LastBlockSize < FreeBlock::kMinBlockSize)
            return false;

        auto DeallocationRange = MemoryRange {SPA, EPA};

        if (m_Memory.Free(DeallocationRange.Begin()))
        {
            if (SPA_Alignment >= FreeBlock::kMinBlockSize)
            {
                auto FirstBlock = FreeBlock::New({block, (size_t) SPA_Alignment});

                AddBlock(FirstBlock);

                ZN_LOG(LogTLSF_Allocator, ELogVerbosity::Verbose, "\t TLSFAllocator::AddFirstBlock: %p", block);
            }

            if (LastBlockSize >= FreeBlock::kMinBlockSize)
            {
                auto LastBlock = FreeBlock::New({EPA, (size_t) LastBlockSize});

                AddBlock(LastBlock);

                ZN_LOG(LogTLSF_Allocator, ELogVerbosity::Verbose, "\t TLSFAllocator::AddSecondBlock: %p", EPA);
            }

            return true;
        }
    }
#endif
    return false;
}
} // namespace Zn

#undef ZN_LOG
#pragma pop_macro("ZN_LOG")

#undef ENABLE_MEM_VERIFY
#undef TLSF_ENABLE_DECOMMIT
