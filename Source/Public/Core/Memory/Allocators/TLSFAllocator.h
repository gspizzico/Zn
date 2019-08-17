#pragma once
#include <array>
#include "Core/Memory/VirtualMemory.h"

namespace Zn
{
	class TLSFAllocator
	{
	public:

		struct FreeBlock
		{
		public:

			static constexpr size_t kFooterSize = sizeof(size_t) + sizeof(uintptr_t) * 2;

			static constexpr size_t kMinBlockSize = kFooterSize + sizeof(size_t);
			
			// The footer contains the size of the block and a pointer to the previous and the next physical block.
			struct Footer
			{
				size_t		m_Size;
				FreeBlock*	m_Previous;
				FreeBlock*	m_Next;
			};

			FreeBlock(size_t blockSize, FreeBlock* const previous, FreeBlock* const next);

			~FreeBlock();

			Footer* GetFooter();

			FreeBlock* Previous();

			FreeBlock* Next();

			size_t Size() const { return m_BlockSize; }

			static Footer* GetPreviousBlockFooter(void* next);

		private:

			static constexpr bool kMarkFreeOnDelete = false;
			
			size_t m_BlockSize;
		};

		static constexpr size_t kNumberOfPools = 8;

		static constexpr size_t kNumberOfLists = 16;

		static constexpr size_t kExponentNumberOfList = 4;

		TLSFAllocator();

		void* Allocate(size_t size, size_t alignment = 1);

		//bool Free(void* address);

		using index_type = unsigned long;

		bool MappingInsert(size_t size, index_type& o_fl, index_type& o_sl);
		bool MappingSearch(size_t size, index_type& o_fl, index_type& o_sl);
	private:


		MemoryResource m_SmallMemory;

		std::array<std::array<FreeBlock*, kNumberOfLists> , kNumberOfPools> m_SmallMemoryPools;

		//~TLSFAllocator();
	};
}
