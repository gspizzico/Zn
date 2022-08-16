#include <Znpch.h>
#include "Core/Memory/Allocators/StackAllocator.h"
#include "Core/Memory/Memory.h"
#include "Core/Memory/VirtualMemory.h"

namespace Zn
{
	StackAllocator::StackAllocator(size_t capacity)
		: m_Memory(std::make_shared<VirtualMemoryRegion>(VirtualMemory::AlignToPageSize(capacity)))
		, m_TopAddress(m_Memory->Begin())
		, m_NextUncommitedAddress(m_TopAddress)
		, m_LastSavedStatus(nullptr)
	{}

	StackAllocator::StackAllocator(StackAllocator&& allocator) noexcept
		: m_Memory(std::move(allocator.m_Memory))
		, m_TopAddress(allocator.m_TopAddress)
		, m_NextUncommitedAddress(allocator.m_NextUncommitedAddress)
		, m_LastSavedStatus(nullptr)
	{
		allocator.m_TopAddress = nullptr;
		allocator.m_NextUncommitedAddress = nullptr;
		allocator.m_LastSavedStatus = nullptr;
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

		_ASSERT(m_Memory->Range().Contains(m_TopAddress));												// OOM guard.

		if (const auto UncommittedMemory = Memory::GetDistance(m_TopAddress, m_NextUncommitedAddress); UncommittedMemory > 0)
		{
			const size_t CommitSize = VirtualMemory::AlignToPageSize(UncommittedMemory);

			VirtualMemory::Commit(m_NextUncommitedAddress, CommitSize);

			m_NextUncommitedAddress = Memory::AddOffset(m_NextUncommitedAddress, CommitSize);
		}

		MemoryDebug::MarkUninitialized(CurrentAddress, m_TopAddress);

		return CurrentAddress;
	}

	bool StackAllocator::Free(void* address)
	{
		_ASSERT(m_Memory->Range().Contains(address));													// Verify that is a valid address

		if (Memory::GetDistance(m_TopAddress, address) > 0)
		{
			MemoryDebug::MarkFree(address, m_TopAddress);

			m_TopAddress = address;																		// New top of the stack ptr.

			return true;
		}

		return false;
	}

	bool StackAllocator::Free()
	{
		MemoryDebug::MarkFree(m_Memory->Begin(), m_TopAddress);													// Decommit all pages.

		m_TopAddress = m_Memory->Begin();

		VirtualMemory::Decommit(m_Memory->Begin(), Memory::GetDistance(m_NextUncommitedAddress, m_Memory->Begin()));

		return true;
	}

	bool StackAllocator::IsAllocated(void* address) const
	{
		return Memory::GetDistance(m_TopAddress, address) > 0;
	}

	size_t StackAllocator::GetCommittedMemory() const
	{
		return Memory::GetDistance(m_NextUncommitedAddress, m_Memory->Begin());
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