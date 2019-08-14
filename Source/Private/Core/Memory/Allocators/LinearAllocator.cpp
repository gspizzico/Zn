#include "Core/Memory/Allocators/LinearAllocator.h"
#include "Core/Memory/Memory.h"
#include "Core/Memory/VirtualMemory.h"

Zn::LinearAllocator::LinearAllocator(size_t capacity, size_t alignment)
	: m_Memory(capacity, alignment)
	, m_NextPageAddress(*m_Memory)
	, m_Address(*m_Memory)
{
}

Zn::LinearAllocator::~LinearAllocator()
{
    Free();
}

void* Zn::LinearAllocator::Allocate(size_t size, size_t alignment)
{
    _ASSERT(size > 0);

    auto AlignedAddress = Memory::Align(m_Address, alignment);

    m_Address = Memory::AddOffset(AlignedAddress, size);

	_ASSERT(m_Memory.Range().Contains(m_Address));

    auto NextPage = Memory::Align(m_Address, VirtualMemory::GetPageSize());

    if (auto AllocationSize = Memory::GetDistance(NextPage, m_NextPageAddress); AllocationSize > 0)
    {
        VirtualMemory::Commit(m_NextPageAddress, static_cast<size_t>(AllocationSize));

        m_NextPageAddress = NextPage;
    }

    MemoryDebug::MarkUninitialized(AlignedAddress, m_Address);

    return AlignedAddress;
}

bool Zn::LinearAllocator::Free()
{
    if (m_Memory)
        return false;

    VirtualMemory::Decommit(*m_Memory, static_cast<size_t>(Memory::GetDistance(m_NextPageAddress, *m_Memory)));
    m_Address = *m_Memory;
    m_NextPageAddress = m_Address;
    
    return true;
}