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
				static constexpr int64_t kValidationPattern = int64_t(0xff5aff5a) << 32ull;
				static constexpr int64_t kValidationMask = 0xffffffff;

			private:
				int64_t		m_Pattern;

			public:
				Footer(size_t size, FreeBlock* const previous, FreeBlock* const next);

				FreeBlock*	m_Previous;
				FreeBlock*	m_Next;

				FreeBlock*	GetBlock() const;

				size_t		GetSize() const;

				bool		IsValid() const;
			};

			static constexpr size_t kFooterSize = sizeof(Footer);

			static constexpr size_t kMinBlockSize = kFooterSize + sizeof(size_t);

			FreeBlock(size_t blockSize, FreeBlock* const previous, FreeBlock* const next);

			~FreeBlock();

			Footer* GetFooter();

			FreeBlock* Previous();

			FreeBlock* Next();

			size_t Size() const { return m_BlockSize; }

			static Footer* GetPreviousBlockFooter(void* next);

			void DumpChain();

		private:

			static constexpr bool kMarkFreeOnDelete = false;
			
			size_t m_BlockSize;
		};

		static constexpr size_t kNumberOfPools = 8;

		static constexpr size_t kNumberOfLists = 16;

		static constexpr size_t kExponentNumberOfList = 4;

		static constexpr size_t kStartFl = 5;			// log2 of kMinBlockSize

		static constexpr size_t kJ = 4;

		void DumpArrays();

		TLSFAllocator();

		void* Allocate(size_t size, size_t alignment = 1);

		bool Free(void* address);

		using index_type = unsigned long;

		bool MappingInsert(size_t size, index_type& o_fl, index_type& o_sl);
		
		bool MappingSearch(size_t& size, index_type& o_fl, index_type& o_sl);

		bool FindSuitableBlock(index_type& fl, index_type& sl);

		FreeBlock* MergePrevious(FreeBlock* block);
		FreeBlock* MergeNext(FreeBlock* block);

	private:

		void RemoveBlock(FreeBlock* block);
		void AddBlock(FreeBlock* block);
		
		static constexpr index_type kFlIndexOffset = 5;

		LinearAllocator m_SmallMemory;

		std::array<std::array<FreeBlock*, kNumberOfLists> , kNumberOfPools> m_SmallMemoryPools;

		uint16_t							 FL_Bitmap;
		std::array<uint16_t, kNumberOfPools> SL_Bitmap;

		//~TLSFAllocator();
	};
}
