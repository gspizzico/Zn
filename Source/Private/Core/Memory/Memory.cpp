#include "Core/Memory/Memory.h"
#include "Core/HAL/PlatformTypes.h"
#include "Core/Math/Math.h"

namespace Zn
{
    MemoryStatus Memory::GetMemoryStatus()
    {
        return PlatformMemory::GetMemoryStatus();
    }
	
	uintptr_t Memory::Align(uintptr_t bytes, size_t alignment)
	{
		const size_t mask = alignment - 1;
		return (bytes + mask) & ~mask;
	}

    void* Memory::Align(void* address, size_t alignment)
    {   
		return reinterpret_cast<void*>(Memory::Align(reinterpret_cast<uintptr_t>(address), alignment));
    }

    bool Memory::IsAligned(void * address, size_t alignment)
    {
        return reinterpret_cast<uintptr_t>(address) % alignment == 0;
    }
    void* Memory::AddOffset(void* address, size_t offset)
    {
        return reinterpret_cast<uintptr_t*>((uintptr_t) address + (uintptr_t) offset);
    }
    void * Memory::SubOffset(void * address, size_t offset)
    {
        return reinterpret_cast<uintptr_t*>((uintptr_t) address - (uintptr_t) offset);
    }
    ptrdiff_t Memory::GetDistance(const void * first, const void * second)
    {
        return reinterpret_cast<intptr_t>(first) - reinterpret_cast<intptr_t>(second);
    }

	void Memory::MarkMemory(void* begin, void* end, int8_t pattern)
	{
		std::fill(
			reinterpret_cast<int8_t*>(begin),
			reinterpret_cast<int8_t*>(end),
			pattern);
	}

    void MemoryDebug::MarkUninitialized(void * begin, void * end)
    {
#if ZN_DEBUG
		Memory::MarkMemory(begin, end, kUninitializedMemoryPattern);
#endif
    }
    void MemoryDebug::MarkFree(void * begin, void * end)
    {
#if ZN_DEBUG
		Memory::MarkMemory(begin, end, kFreeMemoryPattern);
#endif
    }
	void MemoryDebug::TrackAllocation(void* address, size_t size)
	{
		PlatformMemory::TrackAllocation(address, size);
	}
	
	void MemoryDebug::TrackDeallocation(void* address)
	{
		PlatformMemory::TrackDeallocation(address);
	}

	MemoryRange::MemoryRange(MemoryRange&& other)
		: m_Begin(other.m_Begin)
		, m_End(other.m_End)
	{
		other.m_Begin	= nullptr;
		other.m_End		= nullptr;
	}
}

void* operator new(size_t size)
{
    return malloc(size);
}

void* operator new[](size_t size)
{
    return malloc(size);
}

void operator delete (void* mem)
{
    free(mem);
}

void operator delete[](void* mem)
{
    free(mem);
}
