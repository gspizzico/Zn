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

	size_t CalculateMemoryToReserve()
	{
		size_t Total = 0;
		for (size_t i = TLSFAllocator::kExponentNumberOfList; i < TLSFAllocator::kNumberOfPools + TLSFAllocator::kExponentNumberOfList; ++i)
		{
			Total += pow(2, i);
		}
		return Total;
	}

	TLSFAllocator::TLSFAllocator()
		: m_SmallMemory(CalculateMemoryToReserve(), FreeBlock::kMinBlockSize)
	{
	}

	void* TLSFAllocator::Allocate(size_t size, size_t alignment)
	{
		return nullptr;
	}
	bool TLSFAllocator::MappingInsert(size_t size, index_type& o_fl, index_type& o_sl)
	{	
		_BitScanReverse64(&o_fl, size);
		o_sl = size < FreeBlock::kMinBlockSize ? 0 : (size >> (o_fl - kExponentNumberOfList)) - pow(2, kExponentNumberOfList);
		ZN_LOG(LogTLSF_Allocator, ELogVerbosity::Log, "Size %u \t fl %u \t sl %u", size, o_fl, o_sl);
		return true;
	}
	bool TLSFAllocator::MappingSearch(size_t size, index_type& o_fl, index_type& o_sl)
	{
		ZN_LOG(LogTLSF_Allocator, ELogVerbosity::Log, "SEARCH");
		if (size >= FreeBlock::kMinBlockSize)
		{
			index_type InitialSlot = 0;
			_BitScanReverse64(&InitialSlot, size);
			size += (1ll << (InitialSlot - kExponentNumberOfList)) - 1;
			return MappingInsert(size, o_fl, o_sl);
		}

		return MappingInsert(size, o_fl, o_sl);
	}
}