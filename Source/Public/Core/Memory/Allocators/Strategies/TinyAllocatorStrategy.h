#pragma once

#include <Core/Memory/Allocators/PageAllocator.h>
#include <Core/Containers/Vector.h>

#include <array>

namespace Zn
{
	class TinyAllocatorStrategy
	{
	public:

		TinyAllocatorStrategy(size_t capacity);

		void* Allocate(size_t size, size_t alignment = sizeof(void*));

		void Free(void* address, size_t size);

	private:

		struct FreeBlock
		{
			FreeBlock*	next_ = nullptr;
			size_t		size_ = 0;
		};

		size_t GetFreeListIndex(size_t size) const;

		static constexpr auto kFreeBlockSize = sizeof(FreeBlock);

		PageAllocator memory_;

		std::array<FreeBlock*, 16> free_lists_;
		std::array<size_t, 16> num_allocations_;

		size_t num_free_pages_;

		static constexpr size_t kMaxAllocationSize = 16 * 16;
	};
}
