#include "Core/Memory/Allocators/LinearAllocator.h"
#include "Core/Memory/Memory.h"
#include "Core/Memory/VirtualMemory.h"

Zn::LinearAllocator::LinearAllocator(size_t capacity, size_t alignment)
{
    m_MaxAllocatableSize = capacity + (alignment - 1) & ~(alignment -1);
    m_BaseAddress = VirtualMemory::Reserve(m_MaxAllocatableSize);
    m_Address = m_NextPageAddress = m_BaseAddress;
    m_LastAddress = Memory::AddOffset(Memory::Align(m_BaseAddress, alignment), m_MaxAllocatableSize);
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

    _ASSERT(Memory::GetOffset(m_Address, m_LastAddress) <= 0);

    auto NextPage = Memory::Align(m_Address, VirtualMemory::GetPageSize());

    if (auto AllocationSize = Memory::GetOffset(NextPage, m_NextPageAddress); AllocationSize > 0)
    {
        VirtualMemory::Commit(m_NextPageAddress, static_cast<size_t>(AllocationSize));

        m_NextPageAddress = NextPage;
    }

    MemoryDebug::MarkUninitialized(AlignedAddress, m_Address);

    return AlignedAddress;
}

bool Zn::LinearAllocator::Free()
{
    if (m_BaseAddress == nullptr)
        return false;

    VirtualMemory::Decommit(m_BaseAddress, static_cast<size_t>(Memory::GetOffset(m_NextPageAddress, m_BaseAddress)));
    m_Address = m_BaseAddress;
    m_NextPageAddress = m_Address;
    
    return true;
}

bool Zn::LinearAllocator::IsValidAddress(void * address)
{
    return address > m_BaseAddress && address < m_Address;
}
