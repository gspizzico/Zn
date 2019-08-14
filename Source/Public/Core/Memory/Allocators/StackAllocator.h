#pragma once

namespace Zn
{
	class StackAllocator
	{
	public:

		// Stack allocator is not default-constructible nor assignable in any case.
		// Default constructor doesn't make sense because it cannot be modified later.

		// Copy doesn't make sense because it invokes the destructor on the copied element.
		
		// Only move is allowed.

		StackAllocator() = delete;

		StackAllocator(const StackAllocator&) = delete;

		StackAllocator& operator=(const StackAllocator&) = delete;

		StackAllocator(size_t capacity, size_t alignment = 1);

		StackAllocator(StackAllocator&&);

		~StackAllocator();

		// Allocates n @bytes in the stack.
		void* Allocate(size_t bytes);

		// Frees the stack at @address. The new top stack ptr will be @address.
		bool Free(void* address);

		// Wipes out the stack.
		bool Free();

	private:

		size_t m_MaxAllocatableSize		= 0;

		void* m_BaseAddress				= nullptr;

		void* m_LastAddress				= nullptr;
		
		void* m_NextPageAddress			= nullptr;

		void* m_TopAddress				= nullptr;
	};
}
