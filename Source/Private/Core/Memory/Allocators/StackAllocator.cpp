#include "Core/Memory/Allocators/StackAllocator.h"
#include "Core/Memory/Memory.h"
#include "Core/Memory/VirtualMemory.h"
#include "Core/HAL/BasicTypes.h"

namespace Zn
{	
	StackAllocator::StackAllocator(size_t capacity)
		: m_Memory(std::make_shared<VirtualMemoryRegion>(VirtualMemory::AlignToPageSize(capacity)))
		, m_TopAddress(m_Memory->Begin())
		, m_LastSavedStatus(nullptr)
	{
	}

	StackAllocator::StackAllocator(SharedPtr<VirtualMemoryRegion> region)
		: m_Memory(region)
		, m_TopAddress(m_Memory->Begin())
		, m_LastSavedStatus(nullptr)
	{	
	}

	StackAllocator::StackAllocator(StackAllocator&& allocator) noexcept
		: m_Memory(std::move(allocator.m_Memory))
		, m_TopAddress(allocator.m_TopAddress)
		, m_LastSavedStatus(nullptr)
	{	
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

		const MemoryRange Allocation(CurrentAddress, bytes);

		m_TopAddress = Memory::AddOffset(CurrentAddress, bytes);										// Computes the new top stack address.

		_ASSERT(m_Memory->Range().Contains(m_TopAddress));												// OOM guard.

		if (m_CommittedRange.Size() == 0)
		{
			if (VirtualMemory::Commit(CurrentAddress, bytes))
			{
				m_CommittedRange = Allocation;
			}
		}
		else
		{
			if (m_CommittedRange.Contains(Allocation))
			{
				return CurrentAddress;
			}
			else
			{
				VirtualMemory::Commit(m_CommittedRange.End(), Memory::GetDistance(m_TopAddress, m_CommittedRange.End()));

				m_CommittedRange = MemoryRange(m_CommittedRange.Begin(), m_TopAddress);
			}
		}

		_ASSERT(m_CommittedRange.Contains(MemoryRange(CurrentAddress, bytes)));

		MemoryDebug::MarkUninitialized(CurrentAddress, m_TopAddress);

		return CurrentAddress;
	}

	bool StackAllocator::Free(void* address)
	{
		_ASSERT(m_Memory->Range().Contains(address));													// Verify that is a valid address

		if (Memory::GetDistance(address, m_TopAddress) > 0)
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

		VirtualMemory::Decommit(m_CommittedRange.Begin(), m_CommittedRange.Size());

		m_CommittedRange = MemoryRange();

		return true;
	}

	bool StackAllocator::IsAllocated(void* address) const
	{
		return Memory::GetDistance(m_TopAddress, address) > 0;
	}

	size_t StackAllocator::GetCommittedMemory() const
	{
		return m_CommittedRange.Size();
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