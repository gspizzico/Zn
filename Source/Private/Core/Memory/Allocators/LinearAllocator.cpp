#include "Core/Memory/Allocators/LinearAllocator.h"
#include "Core/Memory/Memory.h"
#include "Core/Memory/VirtualMemory.h"
#include "Core/Log/LogMacros.h"

DECLARE_STATIC_LOG_CATEGORY(LogLinearAllocator, ELogVerbosity::Log);

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

	_ASSERT(m_Memory.Range().Contains(m_Address));														// OOM

    auto NextPage = Memory::Align(m_Address, VirtualMemory::GetPageSize());
	
    if (auto AllocationSize = Memory::GetDistance(NextPage, m_NextPageAddress); AllocationSize > 0)
    {	
		ZN_LOG(LogLinearAllocator, ELogVerbosity::Log, "Committing %i bytes. %i bytes left. Usage %.4f", AllocationSize, Memory::GetDistance(m_Memory.Range().End(), NextPage), (float) GetAllocatedMemory() / (float) m_Memory.Size());
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

bool Zn::LinearAllocator::IsAllocated(void* address) const
{
	return m_Memory.Range().Contains(address) && Memory::GetDistance(m_NextPageAddress, address) > 0;
}

size_t Zn::LinearAllocator::GetAllocatedMemory() const
{
	return Memory::GetDistance(m_Address, *m_Memory);
}

size_t Zn::LinearAllocator::GetRemainingMemory() const
{
	return Memory::GetDistance(m_Memory.Range().End(), m_Address);
}
