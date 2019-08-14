#include "Core/Memory/Allocators/StackAllocator.h"
#include "Core/Memory/Memory.h"
#include "Core/Memory/VirtualMemory.h"
#include "Core/HAL/BasicTypes.h"

namespace Zn
{	
	StackAllocator::StackAllocator(size_t capacity, size_t alignment)
		: m_Memory(capacity, alignment)
		, m_NextPageAddress(*m_Memory)
		, m_TopAddress(*m_Memory)
		, m_LastSavedStatus(nullptr)
	{
		VirtualMemory::Commit(*m_Memory, m_Memory.Size());
	}

	StackAllocator::StackAllocator(StackAllocator&& allocator)
		: m_Memory(std::move(allocator.m_Memory))
		, m_NextPageAddress(allocator.m_NextPageAddress)
		, m_TopAddress(allocator.m_TopAddress)
		, m_LastSavedStatus(nullptr)
	{	
		allocator.m_NextPageAddress		= nullptr;
		allocator.m_TopAddress			= nullptr;
	}

	StackAllocator::~StackAllocator()
	{
		Free();
	}

	void* StackAllocator::Allocate(size_t bytes, size_t alignment)
	{
		_ASSERT(bytes > 0);

		auto CurrentAddress = Memory::Align(m_TopAddress, alignment);									// Address to be returned.

		m_TopAddress = Memory::AddOffset(CurrentAddress, bytes);										// Computes the new top stack address.

		_ASSERT(m_Memory.Range().Contains(m_TopAddress));												// OOM guard.

		MemoryDebug::MarkUninitialized(CurrentAddress, m_TopAddress);

		return CurrentAddress;
	}

	bool StackAllocator::Free(void* address)
	{
		_ASSERT(m_Memory.Range().Contains(address));													// Verify that is a valid address

		auto NewTopAddress = address;																	// New top of the stack ptr.

		MemoryDebug::MarkFree(NewTopAddress, m_TopAddress);

		m_TopAddress = NewTopAddress;

		return true;
	}

	bool StackAllocator::Free()
	{
		if (!m_Memory)
			return false;

		MemoryDebug::MarkFree(*m_Memory, m_TopAddress);													// Decommit all pages.

		m_NextPageAddress = m_TopAddress = *m_Memory;

		return true;
	}

	void StackAllocator::SaveStatus()
	{
		auto StatusPointer = Allocate(sizeof(uintptr_t));													// Allocate on the stack an area for the pointer to the previous restore point.

		auto& Status = *reinterpret_cast<uintptr_t*>(StatusPointer);
		Status = reinterpret_cast<uintptr_t>(m_LastSavedStatus);											// Writing on the stack the pointer to the previous restore point.

		m_LastSavedStatus = StatusPointer;
	}
	
	void StackAllocator::RestoreStatus()
	{
		if (m_LastSavedStatus == nullptr)
			return;

		auto PreviousHead = m_TopAddress;

		m_TopAddress = m_LastSavedStatus;

		m_LastSavedStatus = reinterpret_cast<void*>(*reinterpret_cast<uintptr_t*>(m_LastSavedStatus));		// Set the address of the previous restore point.

		MemoryDebug::MarkFree(m_TopAddress, PreviousHead);
	}
}