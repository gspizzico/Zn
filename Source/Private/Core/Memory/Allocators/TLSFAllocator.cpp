#include "Core/Memory/Allocators/TLSFAllocator.h"
#include "Core/Memory/Memory.h"

#include "Core/Log/LogMacros.h"
DECLARE_STATIC_LOG_CATEGORY(LogTLSF_Allocator, ELogVerbosity::Log);

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
		auto FooterPtr = reinterpret_cast<Footer*>(Memory::SubOffset(next, kFooterSize));
		return FooterPtr->m_Size > 0 ? FooterPtr : nullptr;
	}

	void TLSFAllocator::FreeBlock::DumpChain()
	{
		ZN_LOG(LogTLSF_Allocator, ELogVerbosity::Log, "BlockSize %i \t Address %p", m_BlockSize, this);

		if (GetFooter()->m_Next)
		{
			GetFooter()->m_Next->DumpChain();
		}
	}

	size_t CalculateMemoryToReserve()
	{
		size_t Total = 0;
		for (size_t i = TLSFAllocator::kExponentNumberOfList; i < TLSFAllocator::kNumberOfPools + TLSFAllocator::kExponentNumberOfList; ++i)
		{
			Total += static_cast<size_t>(pow(2, i));
		}
		return Total;
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

	TLSFAllocator::TLSFAllocator()
		: m_SmallMemory(1000 * 1000 * 2, FreeBlock::kMinBlockSize)
		, m_SmallMemoryPools()
		, FL_Bitmap(0)
		, SL_Bitmap()
	{
		std::fill(SL_Bitmap.begin(), SL_Bitmap.end(), 0);
	}

	void* TLSFAllocator::Allocate(size_t size, size_t alignment)
	{
		const size_t AllocationSize = size + sizeof(uintptr_t);			// RequestedSize + Sizeof header
		
		index_type fl = 0, sl = 0;
		
		{
			size_t BlockSize = AllocationSize;
			MappingSearch(BlockSize, fl, sl);
		}

		FreeBlock* FreeBlock = nullptr;

		if (!FindSuitableBlock(fl, sl))
		{
			void* AllocatedMemory = m_SmallMemory.Allocate(AllocationSize, Memory::Align(alignment, sizeof(uintptr_t)));
			FreeBlock = new (AllocatedMemory) TLSFAllocator::FreeBlock(AllocationSize, nullptr, nullptr);
		}
		else
		{	
			FreeBlock = m_SmallMemoryPools[fl][sl];

			_ASSERT(FreeBlock != FreeBlock->GetFooter()->m_Next);

			m_SmallMemoryPools[fl][sl] = FreeBlock->GetFooter()->m_Next;

			if (FreeBlock->GetFooter()->m_Next == nullptr)
			{
				// Remove Block
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
			//
		}

		size_t BlockSize = FreeBlock->Size();
		_ASSERT(BlockSize >= AllocationSize);

		int64_t NewBlockSize = (int64_t)BlockSize - (int64_t)AllocationSize;

		if (NewBlockSize >= (FreeBlock::kMinBlockSize))
		{
			BlockSize = AllocationSize;

			TLSFAllocator::FreeBlock* NewBlock = new(Memory::AddOffset(FreeBlock, BlockSize)) TLSFAllocator::FreeBlock{ (size_t) NewBlockSize , nullptr, nullptr };

			index_type in_fl = 0, in_sl = 0;
			MappingInsert(NewBlock->Size(), in_fl, in_sl);

			FL_Bitmap |= (1ull << in_fl);
			SL_Bitmap[in_fl] |= (1ull << in_sl);

			TLSFAllocator::FreeBlock* LastFreeBlock = m_SmallMemoryPools[in_fl][in_sl];

			if (LastFreeBlock != nullptr)
			{
				LastFreeBlock->GetFooter()->m_Previous = NewBlock;
				NewBlock->GetFooter()->m_Next = LastFreeBlock;
			}

			m_SmallMemoryPools[in_fl][in_sl] = NewBlock;
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

		index_type fl = 0, sl = 0;
		MappingInsert(BlockSize, fl, sl);

		TLSFAllocator::FreeBlock* LastFreeBlock = m_SmallMemoryPools[fl][sl];

		auto NewFreeBlock = new (BlockAddress) TLSFAllocator::FreeBlock(BlockSize, nullptr, nullptr);

		if (LastFreeBlock)
		{
			LastFreeBlock->GetFooter()->m_Previous = NewFreeBlock;
			NewFreeBlock->GetFooter()->m_Next = LastFreeBlock;
		}

		m_SmallMemoryPools[fl][sl] = NewFreeBlock;
		
		FL_Bitmap |= (1ull << fl);
		SL_Bitmap[fl] |= (1ull << sl);

		return true;
	}

	bool TLSFAllocator::MappingInsert(size_t size, index_type& o_fl, index_type& o_sl)
	{
		_BitScanReverse64(&o_fl, size);
		o_sl = size < FreeBlock::kMinBlockSize ? 0ul : static_cast<index_type>((size >> (o_fl - kJ)) - pow(2, kJ));
		o_fl -= kStartFl;
		ZN_LOG(LogTLSF_Allocator, ELogVerbosity::Verbose, "Size %u \t fl %u \t sl %u", size, o_fl, o_sl);
		return true;
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
	/*void* TLSFAllocator::MergePrevious(FreeBlock* block)
	{
		if (TLSFAllocator::FreeBlock::Footer* PreviousBlockFooter = TLSFAllocator::FreeBlock::GetPreviousBlockFooter(block))
		{
			TLSFAllocator::FreeBlock* PreviousBlock = PreviousBlockFooter->GetBlock();

		}
	}*/

	TLSFAllocator::FreeBlock* TLSFAllocator::FreeBlock::Footer::GetBlock() const
	{
		return reinterpret_cast<TLSFAllocator::FreeBlock*>(Memory::SubOffset(const_cast<TLSFAllocator::FreeBlock::Footer*>(this), m_Size - sizeof(Footer)));
	}
}