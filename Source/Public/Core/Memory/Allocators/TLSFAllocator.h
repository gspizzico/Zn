#pragma once
#include <array>
#include "Core/Memory/Allocators/PoolAllocator.h"
#include "Core/Memory/VirtualMemory.h"
#include "Core/Math/Math.h"

namespace Zn
{

	class TLSFAllocator
	{
	public:

		static constexpr size_t				kJ = 3;

		static constexpr size_t				kNumberOfPools = 9;

		static constexpr size_t				kNumberOfLists = 1 << kJ;		// pow(2, kJ)

		static constexpr size_t				kStartFl = 8;			// log2(kMinBlockSize)

		static constexpr size_t				kMaxAllocationSize = (1 << (kStartFl + kNumberOfPools - 2));  // 64k is block size, 32 is max allocation size.

		static constexpr size_t				kBlockSize = kMaxAllocationSize * 2;

		struct FreeBlock
		{
		public:
			
			// The footer contains the size of the block and a pointer to the previous and the next physical block.
			struct Footer
			{
			private:
				
				static constexpr int64_t	kValidationPattern	= (int64_t) 0xff5aff5a << 32ull;

				static constexpr int64_t	kValidationMask		= 0xffffffff;

				int64_t						m_Pattern;

			public:

				Footer(size_t size, FreeBlock* const previous, FreeBlock* const next);

				FreeBlock*	GetBlock() const;

				size_t		BlockSize() const;

				bool		IsValid() const;

				FreeBlock*					m_Previous;

				FreeBlock*					m_Next;
			};

			static constexpr size_t			kFooterSize			= sizeof(Footer);

			static constexpr size_t			kMinBlockSize		= std::max(kFooterSize + sizeof(size_t), 1ull << kStartFl);

			static FreeBlock* New(const MemoryRange& block_range);

			FreeBlock(size_t blockSize, FreeBlock* const previous, FreeBlock* const next);

			~FreeBlock();

			Footer*			GetFooter();

			FreeBlock*		Previous();

			FreeBlock*		Next();

			size_t			Size() const { return m_BlockSize; }

			static Footer*	GetPreviousPhysicalFooter(void* block);

#if ZN_DEBUG
			void			LogDebugInfo(bool recursive) const;

			void			Verify(size_t max_block_size) const;

			void			VerifyWrite(MemoryRange range) const;
#endif
		private:

			static constexpr bool			kMarkFreeOnDelete	= false;
			
			size_t							m_BlockSize;
		};

		TLSFAllocator();

		TLSFAllocator(SharedPtr<VirtualMemoryRegion> region, size_t page_size = kMaxAllocationSize);
		
		TLSFAllocator(size_t capacity, size_t page_size = kMaxAllocationSize);

		__declspec(allocator)void*				Allocate(size_t size, size_t alignment = 1);

		bool				Free(void* address);		

		size_t				GetAllocatedMemory() const { return m_Memory.GetUsedMemory(); }

#if ZN_DEBUG
		void				LogDebugInfo() const;

		void				Verify() const;

		void				VerifyWrite(MemoryRange range) const;
#endif

	private:

		using index_type = unsigned long;
		
		using FreeListMatrix = std::array<std::array<FreeBlock*, kNumberOfLists>, kNumberOfPools>;

		bool				MappingInsert(size_t size, index_type& o_fl, index_type& o_sl);

		bool				MappingSearch(size_t size, index_type& o_fl, index_type& o_sl);

		bool				FindSuitableBlock(index_type& o_fl, index_type& o_sl);

		FreeBlock*			MergePrevious(FreeBlock* block);

		FreeBlock*			MergeNext(FreeBlock* block);

		void				RemoveBlock(FreeBlock* block);

		void				AddBlock(FreeBlock* block);

		bool				Decommit(FreeBlock* block);

		MemoryPool								m_Memory;

		FreeListMatrix							m_FreeLists;

		uint16_t								m_FL;

		std::array<uint16_t, kNumberOfPools>	m_SL;
	};
}
