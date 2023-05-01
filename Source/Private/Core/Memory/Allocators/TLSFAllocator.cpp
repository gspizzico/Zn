#include <Znpch.h>
#include "Core/Memory/Allocators/TLSFAllocator.h"
#include "Core/Memory/Memory.h"

DEFINE_STATIC_LOG_CATEGORY(LogTLSF_Allocator, ELogVerbosity::Log);

#define ENABLE_MEM_VERIFY 0
#define TLSF_ENABLE_DECOMMIT 0

#if ENABLE_MEM_VERIFY
#define VERIFY() Verify();
#if ZN_DEBUG
#define VERIFY_WRITE_CALL(MemoryRange) VerifyWrite(MemoryRange);
#else
#define VERIFY_WRITE_CALL(...);
#endif
#define VERIFY_WRITE_PRIVATE(MemoryRange, WriteOperation) VERIFY_WRITE_CALL(MemoryRange); WriteOperation;
#else
#define VERIFY();
#define VERIFY_WRITE_CALL(...);
#define VERIFY_WRITE_PRIVATE(MemoryRange, WriteOperation) WriteOperation;
#endif

#pragma push_macro("ZN_LOG")
#undef ZN_LOG

#define ZN_LOG(...)

namespace Zn
{
#define VERIFY_WRITE(Address, Size, WriteOperationFunction)\
{\
	MemoryRange Range = MemoryRange(Address, Size);\
	VERIFY_WRITE_PRIVATE(Range, WriteOperationFunction(Range.Begin(), Range.End()))\
}

	using FreeBlock = TLSFAllocator::FreeBlock;
	using Footer = FreeBlock::Footer;


	//	=== Footer ====

	Footer::Footer(size_t size, FreeBlock* const previous, FreeBlock* const next)
		: m_Pattern(Footer::kValidationPattern | size)
		, m_Previous(previous)
		, m_Next(next)
	{}

	FreeBlock* Footer::GetBlock() const
	{
		return reinterpret_cast<TLSFAllocator::FreeBlock*>(Memory::SubOffset(const_cast<TLSFAllocator::FreeBlock::Footer*>(this), BlockSize() - sizeof(Footer)));
	}

	size_t Footer::BlockSize() const
	{
		return m_Pattern & kValidationMask;
	}

	bool Footer::IsValid() const
	{
		return (m_Pattern & ~kValidationMask) == kValidationPattern;
	}

	//	===	FreeBlock ===

	FreeBlock* TLSFAllocator::FreeBlock::New(const MemoryRange& block_range)
	{
		MemoryDebug::MarkFree(block_range.Begin(), block_range.End());

		return new (block_range.Begin()) FreeBlock(block_range.Size(), nullptr, nullptr);
	}

	FreeBlock::FreeBlock(size_t blockSize, FreeBlock* const previous, FreeBlock* const next)
		: m_BlockSize(blockSize)
	{
		_ASSERT(blockSize >= kMinBlockSize);

		new (GetFooter()) FreeBlock::Footer{ m_BlockSize, previous, next };
	}

	FreeBlock::~FreeBlock()
	{
		if constexpr (kMarkFreeOnDelete)
		{
			Memory::MarkMemory(this, Memory::AddOffset(this, m_BlockSize), 0);
		}
	}

	Footer* FreeBlock::GetFooter()
	{
		return reinterpret_cast<Footer*>(Memory::AddOffset(this, m_BlockSize - kFooterSize));
	}

	FreeBlock* FreeBlock::Previous()
	{
		return GetFooter()->m_Previous;
	}

	FreeBlock* FreeBlock::Next()
	{
		return GetFooter()->m_Next;
	}

	Footer* FreeBlock::GetPreviousPhysicalFooter(void* block)
	{
		return reinterpret_cast<Footer*>(Memory::SubOffset(block, kFooterSize));
	}

#if ZN_DEBUG
	void FreeBlock::LogDebugInfo(bool recursive) const
	{
		ZN_LOG(LogTLSF_Allocator, ELogVerbosity::Log, "BlockSize %i \t Address %p", m_BlockSize, this);

		if (const auto Next = recursive ? const_cast<FreeBlock*>(this)->GetFooter()->m_Next : nullptr)
		{
			Next->LogDebugInfo(recursive);
		}
	}

	void FreeBlock::Verify(size_t max_block_size) const
	{
		const auto Footer = const_cast<FreeBlock*>(this)->GetFooter();

		_ASSERT(Footer->IsValid());

		if (Footer->m_Next)
		{
			Footer->m_Next->Verify(max_block_size);
		}
	}

	void TLSFAllocator::FreeBlock::VerifyWrite(MemoryRange range) const
	{
		_ASSERT(!range.Contains(this));

		auto Footer = const_cast<FreeBlock*>(this)->GetFooter();
		if (Footer)
		{
			if (auto Next = static_cast<FreeBlock*>(Footer->m_Next))
			{
				Next->VerifyWrite(range);
			}
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
		std::fill(m_SL.begin(), m_SL.end(), 0);
	}

	void* TLSFAllocator::Allocate(size_t size, size_t alignment)
	{
		size_t AllocationSize = Memory::Align(size + sizeof(uintptr_t), FreeBlock::kMinBlockSize);	// FREEBLOCK -> {[BLOCK_SIZE][STORAGE|FOOTER]}

		index_type fl = 0, sl = 0;
		MappingSearch(AllocationSize, fl, sl);

		FreeBlock* FreeBlock = nullptr;

		size_t BlockSize = 0;

		if (!FindSuitableBlock(fl, sl))
		{
			BlockSize = m_Memory.PageSize();
			void* AllocatedMemory = m_Memory.Allocate();											// Allocate a new page if there is no available block for the requested size.
			FreeBlock = FreeBlock::New({ AllocatedMemory, BlockSize });
		}
		else
		{
			FreeBlock = m_FreeLists[fl][sl];														// Pop the block from the list.

			RemoveBlock(FreeBlock);

			BlockSize = FreeBlock->Size();
		}

		_ASSERT(BlockSize >= AllocationSize);

		if (auto NewBlockSize = BlockSize - AllocationSize; NewBlockSize >= (FreeBlock::kMinBlockSize))		// If the free block is bigger than the allocation size, try to create a new smaller block from it.
		{
			BlockSize = AllocationSize;

			auto NewBlockAddress = Memory::AddOffset(FreeBlock, BlockSize);									// BIG FREEBLOCK -> {[REQUESTED_BLOCK][NEW_BLOCK]}

			VERIFY_WRITE(NewBlockAddress, NewBlockSize, MemoryDebug::MarkFree);

			TLSFAllocator::FreeBlock* NewBlock = FreeBlock::New({ NewBlockAddress, NewBlockSize });

			AddBlock(NewBlock);
		}

		VERIFY_WRITE_CALL(MemoryRange(FreeBlock, BlockSize));

		new (FreeBlock) uintptr_t(BlockSize);																// Write at the beginning of the block its size in order to safely free the memory when requested.

		MemoryDebug::TrackAllocation(FreeBlock, BlockSize);

		ZN_LOG(LogTLSF_Allocator, ELogVerbosity::Verbose, "Allocate \t %p, Size: %i", FreeBlock, BlockSize);

		auto AllocationRange = MemoryRange(Memory::AddOffset(FreeBlock, sizeof(uintptr_t)), BlockSize - sizeof(BlockSize));

		VERIFY_WRITE(AllocationRange.Begin(), AllocationRange.Size(), MemoryDebug::MarkUninitialized);

		VERIFY_WRITE(FreeBlock->GetFooter(), FreeBlock::kFooterSize, Memory::Memzero);					   // It's not a free block anymore, footer is unnecessary.

		return AllocationRange.Begin();
	}

	bool TLSFAllocator::Free(void* address)
	{
		if (!m_Memory.IsAllocated(address))
		{
			return false;
		}

		void* BlockAddress = Memory::SubOffset(address, sizeof(uintptr_t));									// Recover this block size

		uintptr_t BlockSize = *reinterpret_cast<uintptr_t*>(BlockAddress);

		ZN_LOG(LogTLSF_Allocator, ELogVerbosity::Verbose, "Free\t %p, Size: %i", BlockAddress, BlockSize);

		VERIFY_WRITE(BlockAddress, BlockSize, MemoryDebug::MarkFree);

		FreeBlock* NewBlock = FreeBlock::New({ BlockAddress, BlockSize });									// Make a new block

		MemoryDebug::TrackDeallocation(BlockAddress);

		NewBlock = MergePrevious(NewBlock);																	// Try merge the previous physical block

		NewBlock = MergeNext(NewBlock);																		// Try merge the next physical block

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

	void TLSFAllocator::Verify() const
	{
		for (int fl = 0; fl < kNumberOfPools; ++fl)
		{
			for (int sl = 0; sl < kNumberOfLists; ++sl)
			{
				if (auto FreeBlock = m_FreeLists[fl][sl])
				{
					const auto Size = static_cast<size_t>(pow(2, fl + kStartFl) + (pow(2, fl + kStartFl) / kNumberOfLists) * (sl + 1ull));

					FreeBlock->Verify(Size);
				}
			}
		}
	}

	void TLSFAllocator::VerifyWrite(MemoryRange range) const
	{
		for (const auto& List : m_FreeLists)
		{
			for (const auto& Head : List)
			{
				if (Head)
				{
					Head->VerifyWrite(range);
				}
			}
		}
	}

#endif

	bool TLSFAllocator::MappingInsert(size_t size, index_type& o_fl, index_type& o_sl)
	{
		if (size <= kMaxAllocationSize)
		{
			_BitScanReverse64(&o_fl, size);																					// FLS -> Find most significant bit and returns log2 of it -> (2^o_fl);

			o_sl = size < FreeBlock::kMinBlockSize ? 0ul : static_cast<index_type>((size >> (o_fl - kJ)) - kNumberOfLists);

			o_fl -= kStartFl;																								// Remove kStartFl because we are not starting from lists of size 2.
		}
		else
		{
			o_fl = kNumberOfPools - 1;
			o_sl = kNumberOfLists - 1;
		}

		ZN_LOG(LogTLSF_Allocator, ELogVerbosity::Verbose, "Size %u \t fl %u \t sl %u", size, o_fl, o_sl);

		return o_fl < kNumberOfPools&& o_sl < kNumberOfLists;
	}

	bool TLSFAllocator::MappingSearch(size_t size, index_type& o_fl, index_type& o_sl)
	{
		o_fl = 0;
		o_sl = 0;

		if (size >= FreeBlock::kMinBlockSize)			// For blocks smaller than the minimum block, use always the minimum block
		{
			index_type fl = 0;

			_BitScanReverse64(&fl, size);

			size += (1ull << (fl - kJ)) - 1;			// Find a block bigger than the appropriate one, in order to correctly fit the obj to be allocated + its header.

			return MappingInsert(size, o_fl, o_sl);
		}

		return true;									// This should always return 0, 0.
	}

	bool TLSFAllocator::FindSuitableBlock(index_type& o_fl, index_type& o_sl)
	{
		// From Implementation of a constant-time dynamic storage allocator -> http://www.gii.upv.es/tlsf/files/spe_2008.pdf

		index_type fl = 0, sl = 0;

		auto Bitmap = m_SL[o_fl] & (0xFFFFFFFFFFFFFFFF << o_sl);

		if (Bitmap != 0)
		{
			_BitScanForward64(&sl, Bitmap);							// FFS -> Find least significant bit log2 (2^sl)

			fl = o_fl;
		}
		else
		{
			Bitmap = m_FL & (0xFFFFFFFFFFFFFFFF << (o_fl + 1));

			if (Bitmap == 0) return false;							// No available blocks have been found. Commit new pages or OOM.

			_BitScanForward64(&fl, Bitmap);

			_BitScanForward64(&sl, m_SL[fl]);
		}

		o_fl = fl;

		o_sl = sl;

		return true;
	}

	TLSFAllocator::FreeBlock* TLSFAllocator::MergePrevious(FreeBlock* block)
	{
		auto PreviousPhysicalBlockFooter = TLSFAllocator::FreeBlock::GetPreviousPhysicalFooter(block);

		if((m_Memory.IsAllocated(PreviousPhysicalBlockFooter) && PreviousPhysicalBlockFooter->IsValid()))
		{
			const auto PreviousBlockSize = PreviousPhysicalBlockFooter->BlockSize();

			FreeBlock* PreviousPhysicalBlock = static_cast<FreeBlock*>(Memory::SubOffset(block, PreviousBlockSize));

			ZN_LOG(LogTLSF_Allocator, ELogVerbosity::Verbose, "MergePrevious \t Block: %p \t Previous: %p", block, PreviousPhysicalBlock);

			RemoveBlock(PreviousPhysicalBlock);

			const auto MergedBlockSize = PreviousBlockSize + block->Size();

			return FreeBlock::New({ PreviousPhysicalBlock, MergedBlockSize });
		}
		else
		{
			return block;
		}
	}

	TLSFAllocator::FreeBlock* TLSFAllocator::MergeNext(FreeBlock* block)
	{
		FreeBlock* NextPhysicalBlock = static_cast<FreeBlock*>(Memory::AddOffset(block, block->Size()));

		if (m_Memory.IsAllocated(NextPhysicalBlock) && NextPhysicalBlock->GetFooter()->IsValid())
		{
			auto NextBlockSize = NextPhysicalBlock->Size();

			ZN_LOG(LogTLSF_Allocator, ELogVerbosity::Verbose, "MergeNext \t Block: %p \t Next: %p", block, NextPhysicalBlock);

			RemoveBlock(NextPhysicalBlock);

			new (NextPhysicalBlock) (uintptr_t)(0);

			auto MergedBlockSize = block->Size() + NextBlockSize;

			return FreeBlock::New({ block, MergedBlockSize });
		}
		return block;
	}

	void TLSFAllocator::RemoveBlock(FreeBlock* block)
	{
		index_type fl = 0, sl = 0;

		MappingInsert(block->Size(), fl, sl);

		ZN_LOG(LogTLSF_Allocator, ELogVerbosity::Verbose, "RemoveBlock \t Block: %p\t  Size: %i, fl %i, sl %i", block, block->Size(), fl, sl);

		auto BlockFooter = block->GetFooter();

		if (m_FreeLists[fl][sl] == block)
		{
			m_FreeLists[fl][sl] = BlockFooter->m_Next;

			if (m_FreeLists[fl][sl] == nullptr)							// Since we are replacing the head, make sure that if it's null we remove this block from FL/SL.
			{
				m_SL[fl] &= ~(1ull << sl);
				if (m_SL[fl] == 0)
				{
					m_FL &= ~(1ull << fl);
				}
			}
			else
			{
				_ASSERT(block == m_FreeLists[fl][sl]->GetFooter()->m_Previous);

				m_FreeLists[fl][sl]->GetFooter()->m_Previous = nullptr;
			}
		}
		else
		{
			_ASSERT(BlockFooter->m_Previous);

			ZN_LOG(LogTLSF_Allocator, ELogVerbosity::Verbose, "\t Previous -> %p", BlockFooter->m_Previous);
			ZN_LOG(LogTLSF_Allocator, ELogVerbosity::Verbose, "\t\t Previous Footer -> %p", BlockFooter->m_Previous->GetFooter());
			ZN_LOG(LogTLSF_Allocator, ELogVerbosity::Verbose, "\t\t\t (Previous Footer).Next -> %p", BlockFooter->m_Previous->GetFooter()->m_Next);

			BlockFooter->m_Previous->GetFooter()->m_Next = BlockFooter->m_Next;			    //Remap previous block to next block			

			if (BlockFooter->m_Next)
			{
				BlockFooter->m_Next->GetFooter()->m_Previous = BlockFooter->m_Previous;
			}

			ZN_LOG(LogTLSF_Allocator, ELogVerbosity::Verbose, "\t\t\t New (Previous Footer).Next -> %p", block->Previous()->GetFooter()->m_Next);
		}

		VERIFY_WRITE(BlockFooter, FreeBlock::kFooterSize, Memory::Memzero);			// Removing it from the map, footer is unnecessary.
	}

	void TLSFAllocator::AddBlock(FreeBlock* block)
	{
		index_type fl = 0, sl = 0;

		MappingInsert(block->Size(), fl, sl);

		ZN_LOG(LogTLSF_Allocator, ELogVerbosity::Verbose, "AddBlock \t Block: %p\t  Size: %i, fl %i, sl %i", block, block->Size(), fl, sl);

		m_FL |= (1ull << fl);
		m_SL[fl] |= (1ull << sl);

		if (auto Head = m_FreeLists[fl][sl]; Head != nullptr)
		{
			ZN_LOG(LogTLSF_Allocator, ELogVerbosity::Verbose, "\t Previous Head -> %p", Head);

			block->GetFooter()->m_Next = Head;

			Head->GetFooter()->m_Previous = block;

			ZN_LOG(LogTLSF_Allocator, ELogVerbosity::Verbose, "\t\t (Previous Head).Previous -> %p", Head->Previous());

		}

#if ENABLE_MEM_VERIFY
		VERIFY_WRITE_CALL(MemoryRange(block, block->Size()));
#endif

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

			if (SPA_Alignment > 0 && SPA_Alignment < FreeBlock::kMinBlockSize) return false;
			if (LastBlockSize > 0 && LastBlockSize < FreeBlock::kMinBlockSize) return false;

			auto DeallocationRange = MemoryRange{ SPA,EPA };

			VERIFY_WRITE_CALL(DeallocationRange);

			{// dbg
				auto pFooter = block->GetFooter();
				_ASSERT(pFooter->m_Next == nullptr);
				_ASSERT(pFooter->m_Previous == nullptr);
			}

			if (m_Memory.Free(DeallocationRange.Begin()))
			{
				if (SPA_Alignment >= FreeBlock::kMinBlockSize)
				{
					auto FirstBlock = FreeBlock::New({ block, (size_t) SPA_Alignment });

					AddBlock(FirstBlock);

					ZN_LOG(LogTLSF_Allocator, ELogVerbosity::Verbose, "\t TLSFAllocator::AddFirstBlock: %p", block);
				}

				if (LastBlockSize >= FreeBlock::kMinBlockSize)
				{
					auto LastBlock = FreeBlock::New({ EPA, (size_t) LastBlockSize });

					AddBlock(LastBlock);

					ZN_LOG(LogTLSF_Allocator, ELogVerbosity::Verbose, "\t TLSFAllocator::AddSecondBlock: %p", EPA);
				}

				VERIFY_WRITE_CALL(DeallocationRange);

				return true;
			}
		}
#endif
		return false;
	}
}

#undef ZN_LOG
#pragma pop_macro("ZN_LOG")

#undef ENABLE_MEM_VERIFY
#undef TLSF_ENABLE_DECOMMIT