#pragma once
#include "Core/Memory/VirtualMemory.h"

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

		StackAllocator(size_t capacity);

		StackAllocator(StackAllocator&&) noexcept;

		~StackAllocator();

		// Allocates n @bytes in the stack.
		void* Allocate(size_t bytes, size_t alignment = 1);

		// Frees the stack at @address. The new top stack ptr will be @address.
		bool Free(void* address);

		// Wipes out the stack.
		bool Free();

		// Returns true if the address is within the stack.
		bool IsAllocated(void* address) const;

		// Returns the size of the committed memory.
		size_t GetCommittedMemory() const;

		// Sets a restore point to which is possible to rewind.
		void SaveStatus();

		// Restores the previous set restore point. Multiple call to this function will restore previous restore points.
		void RestoreStatus();

	private:

		SharedPtr<VirtualMemoryRegion>	m_Memory;

		void* m_TopAddress = nullptr;

		void* m_NextUncommitedAddress = nullptr;

		void* m_LastSavedStatus = nullptr;
	};
}
