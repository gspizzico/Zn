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

	void* Memory::AlignToAddress(void* address, void* start_address, size_t alignment)
	{
		auto Distance = GetDistance(address, start_address);
		return AddOffset(start_address, (Distance - Distance % alignment));
	}

	//void* Memory::AlignDown(void* address, size_t alignment)
	//{
	//	return IsAligned(address, alignment) ? address : Memory::SubOffset(Memory::Align(address, alignment), alignment);
	//}

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

	void Memory::Memzero(void* begin, void* end)
	{
		std::memset(begin, 0, static_cast<size_t>(GetDistance(end, begin)));
	}

	void Memory::Memzero(void* begin, size_t size)
	{
		std::memset(begin, 0, size);
	}

	uint64_t Memory::Convert(uint64_t size, StorageUnit convert_to, StorageUnit convert_from)
	{
		return size * uint64_t(convert_from) / uint64_t(convert_to);
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

	bool MemoryRange::operator==(const MemoryRange& other) const
	{
		return m_Begin == other.m_Begin && m_End == other.m_End;
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
