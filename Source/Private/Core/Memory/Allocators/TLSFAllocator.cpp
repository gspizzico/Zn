#include "Core/Memory/Allocators/TLSFAllocator.h"
#include "Core/Memory/Memory.h"

#include "Core/Log/LogMacros.h"

DECLARE_STATIC_LOG_CATEGORY(LogTLSF_Allocator, ELogVerbosity::Log);

#define ENABLE_MEM_VERIFY 0

#if ENABLE_MEM_VERIFY
#define VERIFY() Verify();
#else
#define VERIFY();
#endif

namespace Zn
{
	using FreeBlock = TLSFAllocator::FreeBlock;
	using Footer = FreeBlock::Footer;

	
	//	=== Footer ====
	
	Footer::Footer(size_t size, FreeBlock* const previous, FreeBlock* const next)
		: m_Pattern(Footer::kValidationPattern | size)
		, m_Previous(previous)
		, m_Next(next)
	{
	}

	FreeBlock* Footer::GetBlock() const
	{
		return reinterpret_cast<TLSFAllocator::FreeBlock*>(Memory::SubOffset(const_cast<TLSFAllocator::FreeBlock::Footer*>(this), GetSize() - sizeof(Footer)));
	}

	size_t Footer::GetSize() const
	{
		return m_Pattern & kValidationMask;
	}

	bool Footer::IsValid() const
	{
		return (m_Pattern & ~kValidationMask) == kValidationPattern;
	}

	//	===	FreeBlock ===

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
		_ASSERT(m_BlockSize > 0 && m_BlockSize <= max_block_size);
		
		const auto Footer = const_cast<FreeBlock*>(this)->GetFooter();

		_ASSERT(Footer->IsValid());
		
		if (Footer->m_Next)
		{
			Footer->m_Next->Verify(max_block_size);
		}
	}	
#endif

	//	===	TLSFAllocator ===

	TLSFAllocator::TLSFAllocator()
		: m_HeapAllocator()
		, m_FreeLists()
		, m_FL(0)
		, m_SL()
	{
		std::fill(m_SL.begin(), m_SL.end(), 0);
	}

	void* TLSFAllocator::Allocate(size_t size, size_t alignment)
	{	
		size_t AllocationSize = Memory::Align(size + sizeof(uintptr_t), sizeof(uintptr_t));			// FREEBLOCK -> {[BLOCK_SIZE][STORAGE|FOOTER]}
		
		index_type fl = 0, sl = 0;
		MappingSearch(AllocationSize, fl, sl);

		FreeBlock* FreeBlock = nullptr;

		if (!FindSuitableBlock(fl, sl))																// Allocate page aligned memory if there is no available block for the requested size.
		{
			auto BlockSize = m_HeapAllocator.GetPageSize();
			void* AllocatedMemory = m_HeapAllocator.AllocatePage();
			FreeBlock = new (AllocatedMemory) TLSFAllocator::FreeBlock(BlockSize, nullptr, nullptr);
		}
		else
		{	
			FreeBlock = m_FreeLists[fl][sl];														// Pop the block from the list.

			RemoveBlock(FreeBlock);
		}

		size_t BlockSize = FreeBlock->Size();

		_ASSERT(BlockSize >= AllocationSize);

		if (auto NewBlockSize = BlockSize - AllocationSize; NewBlockSize >= (FreeBlock::kMinBlockSize))		// If the free block is bigger than the allocation size, try to create a new smaller block from it.
		{
			BlockSize = AllocationSize;

			auto NewBlockAddress = Memory::AddOffset(FreeBlock, BlockSize);									// BIG FREEBLOCK -> {[REQUESTED_BLOCK][NEW_BLOCK]}

			MemoryDebug::MarkFree(NewBlockAddress, Memory::AddOffset(NewBlockAddress, NewBlockSize));

			TLSFAllocator::FreeBlock* NewBlock = new(NewBlockAddress) TLSFAllocator::FreeBlock{ NewBlockSize , nullptr, nullptr };

			AddBlock(NewBlock);		
		}
		
		Memory::MarkMemory(FreeBlock, Memory::AddOffset(FreeBlock, BlockSize), 0);							// Clear memory before returning it. Leaving it as it is might leave footer info even when not needed
																											// because the allocated memory is always more than the exact request.

		new(FreeBlock) uintptr_t(BlockSize);																// Write at the beginning of the block its size in order to safely free the memory when requested.
		
		MemoryDebug::TrackAllocation(FreeBlock, BlockSize);

		return Memory::AddOffset(FreeBlock, sizeof(uintptr_t));
	}

	bool TLSFAllocator::Free(void* address)
	{
		void* BlockAddress = Memory::SubOffset(address, sizeof(uintptr_t));									// Recover this block size

		uintptr_t BlockSize = *reinterpret_cast<uintptr_t*>(BlockAddress);

		MemoryDebug::MarkFree(BlockAddress, Memory::AddOffset(BlockAddress, BlockSize));

		FreeBlock* NewBlock = new (BlockAddress) TLSFAllocator::FreeBlock(BlockSize, nullptr, nullptr);		// Make a new block

		MemoryDebug::TrackDeallocation(BlockAddress);

		NewBlock = MergePrevious(NewBlock);																	// Try merge the previous physical block
		
		NewBlock = MergeNext(NewBlock);																		// Try merge the next physical block

		VERIFY();
		
		AddBlock(NewBlock);

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

		return o_fl < kNumberOfPools && o_sl < kNumberOfLists;
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

		if (m_HeapAllocator.IsAllocated(PreviousPhysicalBlockFooter) && PreviousPhysicalBlockFooter->IsValid())
		{
			const auto PreviousBlockSize = PreviousPhysicalBlockFooter->GetSize();

			const auto MergedBlockSize = PreviousBlockSize + block->Size();

			FreeBlock* PreviousPhysicalBlock = static_cast<FreeBlock*>(Memory::SubOffset(block, PreviousBlockSize));
			
			RemoveBlock(PreviousPhysicalBlock);

			MemoryDebug::MarkFree(PreviousPhysicalBlock, Memory::AddOffset(PreviousPhysicalBlock, MergedBlockSize));

			return new(PreviousPhysicalBlock) TLSFAllocator::FreeBlock(MergedBlockSize, nullptr, nullptr);
		}
		else
		{
			return block;
		}
	}

	TLSFAllocator::FreeBlock* TLSFAllocator::MergeNext(FreeBlock* block)
	{
		if (FreeBlock* NextPhysicalBlock = static_cast<FreeBlock*>(Memory::AddOffset(block, block->Size())); m_HeapAllocator.IsAllocated(NextPhysicalBlock) && NextPhysicalBlock->GetFooter()->IsValid())
		{
			auto NextBlockSize = NextPhysicalBlock->Size();

			auto MergedBlockSize = block->Size() + NextBlockSize;			

			RemoveBlock(NextPhysicalBlock);

			MemoryDebug::MarkFree(block, Memory::AddOffset(block, MergedBlockSize));

			return new (block) TLSFAllocator::FreeBlock(MergedBlockSize, nullptr, nullptr);
		}
		return block;
	}

	void TLSFAllocator::RemoveBlock(FreeBlock* block)
	{	
		index_type fl = 0, sl = 0;

		MappingInsert(block->Size(), fl, sl);

		if (m_FreeLists[fl][sl] == block)
		{
			m_FreeLists[fl][sl] = block->Next();

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
			_ASSERT(block->Previous());

			ZN_LOG(LogTLSF_Allocator, ELogVerbosity::Verbose, "Removing %p from %i fl, %i sl.", block, fl, sl);
			ZN_LOG(LogTLSF_Allocator, ELogVerbosity::Verbose, "\t Previous -> %p", block->Previous());
			ZN_LOG(LogTLSF_Allocator, ELogVerbosity::Verbose, "\t\t Previous Footer -> %p", block->Previous()->GetFooter());
			ZN_LOG(LogTLSF_Allocator, ELogVerbosity::Verbose, "\t\t\t Previous Footer Next -> %p", block->Previous()->GetFooter()->m_Next);
			
			if (block->Next())
			{
				block->Next()->GetFooter()->m_Previous = block->Previous();
			}

			block->Previous()->GetFooter()->m_Next = block->Next();			    //Remap previous block to next block			

			ZN_LOG(LogTLSF_Allocator, ELogVerbosity::Verbose, "\t\t\t Previous Footer Next AFTER-> %p", block->Previous()->GetFooter()->m_Next);
		}
	}

	void TLSFAllocator::AddBlock(FreeBlock* block)
	{	
		index_type fl = 0, sl = 0;

		MappingInsert(block->Size(), fl, sl);

		ZN_LOG(LogTLSF_Allocator, ELogVerbosity::Verbose, "Adding %p to %i fl, %i sl.", block, fl, sl);

		m_FL |= (1ull << fl);
		m_SL[fl] |= (1ull << sl);

		if (auto Head = m_FreeLists[fl][sl]; Head != nullptr)
		{
			ZN_LOG(LogTLSF_Allocator, ELogVerbosity::Verbose, "\t Head -> %p", Head);

			block->GetFooter()->m_Next = Head;

			Head->GetFooter()->m_Previous = block;

			ZN_LOG(LogTLSF_Allocator, ELogVerbosity::Verbose, "\t\t Head Previous AFTER -> %p", Head->Previous());

		}

		m_FreeLists[fl][sl] = block;

		ZN_LOG(LogTLSF_Allocator, ELogVerbosity::Verbose, "\t Next -> %p", block->Next());
	}
}

#undef ENABLE_MEM_VERIFY