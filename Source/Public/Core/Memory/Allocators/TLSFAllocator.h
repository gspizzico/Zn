#pragma once
#include <array>
#include "Core/Memory/Allocators/LinearAllocator.h"
#include "Core/Memory/VirtualMemory.h"
#include "Core/Math/Math.h"

namespace Zn
{

	class TLSFAllocator
	{
	public:

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

				size_t		GetSize() const;

				bool		IsValid() const;

				FreeBlock*					m_Previous;

				FreeBlock*					m_Next;
			};

			static constexpr size_t			kFooterSize			= sizeof(Footer);

			static constexpr size_t			kMinBlockSize		= std::max(kFooterSize + sizeof(size_t), 128ull);

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
#endif
		private:

			static constexpr bool			kMarkFreeOnDelete	= false;
			
			size_t							m_BlockSize;
		};

		static constexpr size_t				kJ = 3;

		static constexpr size_t				kNumberOfPools		= 7;

		static constexpr size_t				kNumberOfLists		= 8;			// pow(2, kJ)

		static constexpr size_t				kStartFl			= 7;			// log2(kMinBlockSize)


		TLSFAllocator(size_t capacity);

		__declspec(allocator)void*				Allocate(size_t size, size_t alignment = 1);

		bool				Free(void* address);		

		size_t				GetAllocatedMemory() const { return m_InternalAllocator.GetAllocatedMemory(); }

		size_t				GetCapacity()		 const { return m_InternalAllocator.GetMemory().Size(); }

		static size_t		GetMaxAllocatableBlockSize();

#if ZN_DEBUG
		void				LogDebugInfo() const;

		void				Verify() const;
#endif

	private:

		using index_type = unsigned long;

		bool				MappingInsert(size_t size, index_type& o_fl, index_type& o_sl);

		bool				MappingSearch(size_t size, index_type& o_fl, index_type& o_sl);

		bool				FindSuitableBlock(index_type& o_fl, index_type& o_sl);

		FreeBlock*			MergePrevious(FreeBlock* block);

		FreeBlock*			MergeNext(FreeBlock* block);

		void				RemoveBlock(FreeBlock* block);

		void				AddBlock(FreeBlock* block);

		LinearAllocator						m_InternalAllocator;

		std::array<std::array<FreeBlock*, kNumberOfLists> , kNumberOfPools> m_FreeLists;

		uint16_t							 m_FL;

		std::array<uint16_t, kNumberOfPools> m_SL;
	};
}
