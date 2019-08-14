#include "Core/Memory/Allocators/StackAllocator.h"
#include "Core/Memory/Memory.h"
#include "Core/Memory/VirtualMemory.h"
#include "Core/HAL/BasicTypes.h"

namespace Zn
{	
	StackAllocator::StackAllocator(size_t capacity, size_t alignment)
		: m_MaxAllocatableSize(capacity + (alignment -1) & ~(alignment -1))
		, m_BaseAddress(VirtualMemory::Reserve(m_MaxAllocatableSize))
		, m_LastAddress(Memory::AddOffset(Memory::Align(m_BaseAddress, alignment), m_MaxAllocatableSize))
		, m_NextPageAddress(m_BaseAddress)
		, m_TopAddress(m_BaseAddress)
	{	
	}

	StackAllocator::StackAllocator(StackAllocator&& allocator)
	{
		m_MaxAllocatableSize			= allocator.m_MaxAllocatableSize;
		m_BaseAddress					= allocator.m_BaseAddress;
		m_LastAddress					= allocator.m_LastAddress;
		m_NextPageAddress				= allocator.m_NextPageAddress;
		m_TopAddress					= allocator.m_TopAddress;

		allocator.m_MaxAllocatableSize	= 0;
		allocator.m_BaseAddress			= nullptr;
		allocator.m_LastAddress			= nullptr;
		allocator.m_NextPageAddress		= nullptr;
		allocator.m_TopAddress			= nullptr;
	}

	StackAllocator::~StackAllocator()
	{
		Free();
	}

	void* StackAllocator::Allocate(size_t bytes)
	{
		_ASSERT(bytes > 0);

		auto CurrentAddress = m_TopAddress;																// Address to be returned.

		m_TopAddress = Memory::AddOffset(m_TopAddress, bytes);											// Computes the new top stack address.

		_ASSERT(Memory::GetOffset(m_TopAddress, m_LastAddress) <= 0);									// OOM guard.

		auto NextPage = Memory::Align(m_TopAddress, VirtualMemory::GetPageSize());						// Align the address to page size to check if another page is needed.

		if (auto AllocationSize = Memory::GetOffset(NextPage, m_NextPageAddress); AllocationSize > 0)
		{
			VirtualMemory::Commit(m_NextPageAddress, static_cast<uint64>(AllocationSize));				// Commit a page from Virtual Memory.

			m_NextPageAddress = NextPage;
		}

		MemoryDebug::MarkUninitialized(CurrentAddress, m_TopAddress);

		return CurrentAddress;
	}

	bool StackAllocator::Free(void* address)
	{
		_ASSERT(address >= m_BaseAddress && address <= m_LastAddress);									// Verify that is a valid address

		auto NewTopAddress = address;																	// New top of the stack ptr.

		MemoryDebug::MarkFree(NewTopAddress, m_TopAddress);

		// If the free memory between the new stack ptr and the last committed address is greater than page size, we can free up at least one page.
		if (auto FreeMemory = Memory::GetOffset(m_NextPageAddress, NewTopAddress); static_cast<size_t>(FreeMemory) >= VirtualMemory::GetPageSize())
		{
			auto PagesToDecommit = static_cast<size_t>(FreeMemory) / VirtualMemory::GetPageSize();

			auto NextPage = Memory::SubOffset(m_NextPageAddress, PagesToDecommit * VirtualMemory::GetPageSize());

			bool Success = VirtualMemory::Decommit(NextPage, Memory::GetOffset(m_NextPageAddress, NextPage));

			_ASSERT(Success);

			m_NextPageAddress = NextPage;
		}

		m_TopAddress = NewTopAddress;

		return true;
	}

	bool StackAllocator::Free()
	{
		if (m_BaseAddress == nullptr)
			return false;

		// Decommit all pages.
		VirtualMemory::Decommit(m_BaseAddress, static_cast<size_t>(Memory::GetOffset(m_NextPageAddress, m_BaseAddress)));
		m_NextPageAddress = m_TopAddress = m_BaseAddress;

		return true;
	}
}