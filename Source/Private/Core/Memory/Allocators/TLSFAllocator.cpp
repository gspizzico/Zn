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
	TLSFAllocator::FreeBlock::FreeBlock(size_t blockSize, FreeBlock* const previous, FreeBlock* const next)
		: m_BlockSize(blockSize)
	{
		_ASSERT(blockSize >= kMinBlockSize);

		new (GetFooter()) FreeBlock::Footer{ m_BlockSize, previous, next };
	}

	TLSFAllocator::FreeBlock::~FreeBlock()
	{
		if constexpr (kMarkFreeOnDelete)
		{
			MemoryDebug::MarkMemory(this, Memory::AddOffset(this, m_BlockSize), 0);
		}
	}

	TLSFAllocator::FreeBlock::Footer* TLSFAllocator::FreeBlock::GetFooter()
	{
		return reinterpret_cast<Footer*>(Memory::AddOffset(this, m_BlockSize - kFooterSize));
	}

	TLSFAllocator::FreeBlock* TLSFAllocator::FreeBlock::Previous()
	{
		return GetFooter()->m_Previous;
	}

	TLSFAllocator::FreeBlock* TLSFAllocator::FreeBlock::Next()
	{
		return GetFooter()->m_Next;
	}

	TLSFAllocator::FreeBlock::Footer* TLSFAllocator::FreeBlock::GetPreviousBlockFooter(void* next)
	{
		return reinterpret_cast<Footer*>(Memory::SubOffset(next, kFooterSize));
	}

	TLSFAllocator::FreeBlock::Footer::Footer(size_t size, FreeBlock* const previous, FreeBlock* const next)
		: m_Pattern(Footer::kValidationPattern | size)
		, m_Previous(previous)
		, m_Next(next)
	{
	}

	TLSFAllocator::FreeBlock* TLSFAllocator::FreeBlock::Footer::GetBlock() const
	{
		return reinterpret_cast<TLSFAllocator::FreeBlock*>(Memory::SubOffset(const_cast<TLSFAllocator::FreeBlock::Footer*>(this), GetSize() - sizeof(Footer)));
	}

	size_t TLSFAllocator::FreeBlock::Footer::GetSize() const
	{
		return m_Pattern & kValidationMask;
	}

	bool TLSFAllocator::FreeBlock::Footer::IsValid() const
	{
		return (m_Pattern & ~kValidationMask) == kValidationPattern;
	}

	void TLSFAllocator::FreeBlock::DumpChain()
	{
		ZN_LOG(LogTLSF_Allocator, ELogVerbosity::Log, "BlockSize %i \t Address %p", m_BlockSize, this);

		if (GetFooter()->m_Next)
		{
			GetFooter()->m_Next->DumpChain();
		}
	}

	bool TLSFAllocator::FreeBlock::Verify(size_t max_block_size)
	{
		_ASSERT(m_BlockSize > 0 && m_BlockSize <= max_block_size);
		_ASSERT(GetFooter()->IsValid());
		_ASSERT(GetFooter()->m_Next ? GetFooter()->m_Next->Verify(max_block_size) : true);
		return true;
	}	

	void TLSFAllocator::DumpArrays()
	{
		for (int fl = 0; fl < kNumberOfPools; ++fl)
		{
			for (int sl = 0; sl < kNumberOfLists; ++sl)
			{
				ZN_LOG(LogTLSF_Allocator, ELogVerbosity::Log, "FL \t %i \t SL \t %i", fl, sl);

				if (auto FreeBlock = m_SmallMemoryPools[fl][sl])
				{
					FreeBlock->DumpChain();
				}
			}
		}
	}

	void TLSFAllocator::Verify()
	{
		for (int fl = 0; fl < kNumberOfPools; ++fl)
		{
			for (int sl = 0; sl < kNumberOfLists; ++sl)
			{
				auto Size = pow(2, fl + kFlIndexOffset) + (pow(2, fl + kFlIndexOffset) / kNumberOfLists) * (sl + 1);
				if (auto FreeBlock = m_SmallMemoryPools[fl][sl])
				{
					FreeBlock->Verify(Size);
				}
			}
		}
	}

	TLSFAllocator::TLSFAllocator(size_t capacity)
		: m_SmallMemory(capacity, FreeBlock::kMinBlockSize)
		, m_SmallMemoryPools()
		, FL_Bitmap(0)
		, SL_Bitmap()
	{
		std::fill(SL_Bitmap.begin(), SL_Bitmap.end(), 0);
	}

	void* TLSFAllocator::Allocate(size_t size, size_t alignment)
	{
		//size_t AllocationSize = Memory::Align(size + sizeof(uintptr_t), FreeBlock::kMinBlockSize);			// RequestedSize + Sizeof header
		size_t AllocationSize = Memory::Align(size + sizeof(uintptr_t), sizeof(uintptr_t));			// RequestedSize + Sizeof header
		//size_t AllocationSize = size + sizeof(uintptr_t);			// RequestedSize + Sizeof header
		
		index_type fl = 0, sl = 0;
		{
			size_t block_size = AllocationSize;
			MappingSearch(block_size, fl, sl);
		}

		FreeBlock* FreeBlock = nullptr;

		if (!FindSuitableBlock(fl, sl))
		{
			auto BlockSize = VirtualMemory::AlignToPageSize(AllocationSize);
			void* AllocatedMemory = m_SmallMemory.Allocate(BlockSize);
			FreeBlock = new (AllocatedMemory) TLSFAllocator::FreeBlock(BlockSize, nullptr, nullptr);
		}
		else
		{	
			FreeBlock = m_SmallMemoryPools[fl][sl];

			RemoveBlock(FreeBlock);
		}

		size_t BlockSize = FreeBlock->Size();

		_ASSERT(BlockSize >= AllocationSize);

		if (auto NewBlockSize = BlockSize - AllocationSize; NewBlockSize >= (FreeBlock::kMinBlockSize))
		{
			BlockSize = AllocationSize;
			MemoryDebug::MarkFree(Memory::AddOffset(FreeBlock, BlockSize), Memory::AddOffset(FreeBlock, BlockSize + NewBlockSize));
			TLSFAllocator::FreeBlock* NewBlock = new(Memory::AddOffset(FreeBlock, BlockSize)) TLSFAllocator::FreeBlock{ NewBlockSize , nullptr, nullptr };

			AddBlock(NewBlock);		
		}
		
		MemoryDebug::MarkUninitialized(FreeBlock, Memory::AddOffset(FreeBlock, BlockSize));

		new(FreeBlock) uintptr_t(BlockSize);

		return Memory::AddOffset(FreeBlock, sizeof(uintptr_t));
	}

	bool TLSFAllocator::Free(void* address)
	{
		void* BlockAddress = Memory::SubOffset(address, sizeof(uintptr_t));

		uintptr_t BlockSize = *reinterpret_cast<uintptr_t*>(BlockAddress);

		MemoryDebug::MarkFree(BlockAddress, Memory::AddOffset(BlockAddress, BlockSize));

		FreeBlock* NewBlock = new (BlockAddress) TLSFAllocator::FreeBlock(BlockSize, nullptr, nullptr);

		NewBlock = MergePrevious(NewBlock);
		
		NewBlock = MergeNext(NewBlock);

		VERIFY();
		
		AddBlock(NewBlock);

		return true;
	}

	bool TLSFAllocator::MappingInsert(size_t size, index_type& o_fl, index_type& o_sl)
	{
		_BitScanReverse64(&o_fl, size);
		o_sl = size < FreeBlock::kMinBlockSize ? 0ul : static_cast<index_type>((size >> (o_fl - kJ)) - pow(2, kJ));
		o_fl -= kStartFl;
		ZN_LOG(LogTLSF_Allocator, ELogVerbosity::Verbose, "Size %u \t fl %u \t sl %u", size, o_fl, o_sl);
		return o_fl < m_SmallMemoryPools.size() && o_sl < m_SmallMemoryPools[o_fl].size();
	}

	bool TLSFAllocator::MappingSearch(size_t& size, index_type& o_fl, index_type& o_sl)
	{
		ZN_LOG(LogTLSF_Allocator, ELogVerbosity::Verbose, "SEARCH");
		if (size >= FreeBlock::kMinBlockSize)
		{
			index_type InitialSlot = 0;
			_BitScanReverse64(&InitialSlot, size);
			size += (1ull << (InitialSlot - kJ)) - 1;
			return MappingInsert(size, o_fl, o_sl);
		}

		return MappingInsert(size, o_fl, o_sl);
	}

	bool TLSFAllocator::FindSuitableBlock(index_type& fl, index_type& sl)
	{
		index_type _fl = 0, _sl = 0;

		auto bitmap_tmp = SL_Bitmap[fl] & (0xFFFFFFFFFFFFFFFF << sl);
		if (bitmap_tmp != 0)
		{
			_BitScanForward64(&_sl, bitmap_tmp);
			_fl = fl;
		}
		else
		{
			bitmap_tmp = FL_Bitmap & (0xFFFFFFFFFFFFFFFF << (fl + 1));
			if (bitmap_tmp == 0) return false;
			_BitScanForward64(&_fl, bitmap_tmp);
			_BitScanForward64(&_sl, SL_Bitmap[_fl]);
		}

		fl = _fl;
		sl = _sl;

		return true;
	}

	TLSFAllocator::FreeBlock* TLSFAllocator::MergePrevious(FreeBlock* block)
	{	
		if (auto PreviousPhysicalBlockFooter = TLSFAllocator::FreeBlock::GetPreviousBlockFooter(block); m_SmallMemory.IsAllocated(PreviousPhysicalBlockFooter) && PreviousPhysicalBlockFooter->IsValid())
		{
			const auto PreviousBlockSize = PreviousPhysicalBlockFooter->GetSize();
			const auto MergedBlockSize = PreviousBlockSize + block->Size();
			
			if (MergedBlockSize > pow(2, kStartFl + kNumberOfPools) - 1)
				return block;

			FreeBlock* PreviousPhysicalBlock = static_cast<FreeBlock*>(Memory::SubOffset(block, PreviousBlockSize));
			
			RemoveBlock(PreviousPhysicalBlock);

			MemoryDebug::MarkMemory(PreviousPhysicalBlock, Memory::AddOffset(PreviousPhysicalBlock, MergedBlockSize), 0);

			return new(PreviousPhysicalBlock) TLSFAllocator::FreeBlock(MergedBlockSize, nullptr, nullptr);
		}
		else
		{
			return block;
		}
	}

	TLSFAllocator::FreeBlock* TLSFAllocator::MergeNext(FreeBlock* block)
	{
		if (FreeBlock* NextPhysicalBlock = static_cast<FreeBlock*>(Memory::AddOffset(block, block->Size())); m_SmallMemory.IsAllocated(NextPhysicalBlock) && NextPhysicalBlock->GetFooter()->IsValid())
		{
			auto NextBlockSize = NextPhysicalBlock->Size();
			auto MergedBlockSize = block->Size() + NextBlockSize;

			if (MergedBlockSize > pow(2, kStartFl + kNumberOfPools) - 1)
				return block;

			RemoveBlock(NextPhysicalBlock);

			MemoryDebug::MarkMemory(block, Memory::AddOffset(block, MergedBlockSize), 0);

			return new (block) TLSFAllocator::FreeBlock(MergedBlockSize, nullptr, nullptr);
		}
		return block;
	}

	void TLSFAllocator::RemoveBlock(FreeBlock* block)
	{	
		index_type fl = 0, sl = 0;

		MappingInsert(block->Size(), fl, sl);

		if (m_SmallMemoryPools[fl][sl] == block)
		{
			m_SmallMemoryPools[fl][sl] = block->Next();

			if (m_SmallMemoryPools[fl][sl] == nullptr)							// Since we are replacing the head, make sure that if it's null we remove this block from FL/SL.
			{
				SL_Bitmap[fl] &= ~(1ull << sl);
				if (SL_Bitmap[fl] == 0)
				{
					FL_Bitmap &= ~(1ull << fl);
				}
			}
			else
			{
				m_SmallMemoryPools[fl][sl]->GetFooter()->m_Previous = nullptr;
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

		FL_Bitmap |= (1ull << fl);
		SL_Bitmap[fl] |= (1ull << sl);

		if (auto Head = m_SmallMemoryPools[fl][sl]; Head != nullptr)
		{
			block->GetFooter()->m_Next = Head;

			Head->GetFooter()->m_Previous = block;
		}

		m_SmallMemoryPools[fl][sl] = block;
	}
}

#undef ENABLE_MEM_VERIFY