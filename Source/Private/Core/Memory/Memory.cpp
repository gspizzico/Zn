#include <Znpch.h>
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

// void* Memory::AlignDown(void* address, size_t alignment)
//{
//	return IsAligned(address, alignment) ? address : Memory::SubOffset(Memory::Align(address, alignment), alignment);
// }

bool Memory::IsAligned(void* address, size_t alignment)
{
    return reinterpret_cast<uintptr_t>(address) % alignment == 0;
}
void* Memory::AddOffset(void* address, size_t offset)
{
    return reinterpret_cast<uintptr_t*>((uintptr_t) address + (uintptr_t) offset);
}
void* Memory::SubOffset(void* address, size_t offset)
{
    return reinterpret_cast<uintptr_t*>((uintptr_t) address - (uintptr_t) offset);
}
ptrdiff_t Memory::GetDistance(const void* first, const void* second)
{
    return reinterpret_cast<intptr_t>(first) - reinterpret_cast<intptr_t>(second);
}

void Memory::MarkMemory(void* begin, void* end, int8_t pattern)
{
    std::fill(reinterpret_cast<int8_t*>(begin), reinterpret_cast<int8_t*>(end), pattern);
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

void MemoryDebug::MarkUninitialized(void* begin, void* end)
{
#if ZN_DEBUG
    Memory::MarkMemory(begin, end, kUninitializedMemoryPattern);
#endif
}
void MemoryDebug::MarkFree(void* begin, void* end)
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
    other.m_Begin = nullptr;
    other.m_End   = nullptr;
}

bool MemoryRange::operator==(const MemoryRange& other) const
{
    return m_Begin == other.m_Begin && m_End == other.m_End;
}

bool MemoryRange::Contains(const MemoryRange& other) const
{
    if (*this == other)
        return true;

    const auto DistanceFromStart = Memory::GetDistance(other.Begin(), m_Begin);

    const auto DistanceFromEnd = Memory::GetDistance(m_End, other.End());

    return (DistanceFromStart >= 0 && DistanceFromEnd > 0) || (DistanceFromStart > 0 && DistanceFromEnd >= 0);
}
} // namespace Zn

// Allocators

#include <Core/Memory/Allocators/BaseAllocator.h>

namespace Zn::Allocators
{
// TODO: Make Thread Safe
BaseAllocator* GDefaultAllocator    = nullptr;
BaseAllocator* GAllocator           = nullptr;
bool           GIsCreatingAllocator = false;

int CreateGAllocatorSafe()
{
    GDefaultAllocator = new TrackedMalloc();
    GAllocator        = PlatformMemory::CreateAllocator();
    return 1;
}

void CreateGAllocator()
{
    // Thread-safe, but do we need it?
    static int StaticInstance = CreateGAllocatorSafe();
}

void* New(size_t size, size_t alignment)
{
    if (GAllocator == NULL)
    {
        if (GIsCreatingAllocator == false)
        {
            GIsCreatingAllocator = true;
            CreateGAllocator();
            GIsCreatingAllocator = false;
        }
    }

    auto allocator = GIsCreatingAllocator == false ? GAllocator : GDefaultAllocator;

    auto Address = allocator->Malloc(size, alignment);

    ZN_MEMTRACE_ALLOC(Address, size);

    return Address;
}

void Delete(void* address)
{
    ZN_MEMTRACE_FREE(address);

    bool success = GAllocator && GAllocator->Free(address);

    if (!success)
    {
        success = GDefaultAllocator->Free(address);
    }

    check(success || (address == nullptr));
}
} // namespace Zn::Allocators

using namespace Zn::Allocators;

#pragma warning(push)
#pragma warning(disable : 28251) // microsoft vs code analysis warning

static_assert(__STDCPP_DEFAULT_NEW_ALIGNMENT__ > 0);

void operator delete(void* p) noexcept
{
    Zn::Allocators::Delete(p);
};
void operator delete[](void* p) noexcept
{
    Zn::Allocators::Delete(p);
};

void operator delete(void* p, const std::nothrow_t&) noexcept
{
    Zn::Allocators::Delete(p);
}
void operator delete[](void* p, const std::nothrow_t&) noexcept
{
    Zn::Allocators::Delete(p);
}

void* operator new(std::size_t n) noexcept(false)
{
    return Zn::Allocators::New(n, __STDCPP_DEFAULT_NEW_ALIGNMENT__);
}
void* operator new[](std::size_t n) noexcept(false)
{
    return Zn::Allocators::New(n, __STDCPP_DEFAULT_NEW_ALIGNMENT__);
}

void* operator new(std::size_t n, const std::nothrow_t& tag) noexcept
{
    (void) (tag);
    return Zn::Allocators::New(n, __STDCPP_DEFAULT_NEW_ALIGNMENT__);
}
void* operator new[](std::size_t n, const std::nothrow_t& tag) noexcept
{
    (void) (tag);
    return Zn::Allocators::New(n, __STDCPP_DEFAULT_NEW_ALIGNMENT__);
}

#if (__cplusplus >= 201402L || _MSC_VER >= 1916)
void operator delete(void* p, std::size_t n) noexcept
{
    Zn::Allocators::Delete(p);
};
void operator delete[](void* p, std::size_t n) noexcept
{
    Zn::Allocators::Delete(p);
};
#endif

#if (__cplusplus > 201402L || defined(__cpp_aligned_new))
void operator delete(void* p, std::align_val_t al) noexcept
{
    Zn::Allocators::Delete(p);
}
void operator delete[](void* p, std::align_val_t al) noexcept
{
    Zn::Allocators::Delete(p);
}
void operator delete(void* p, std::size_t n, std::align_val_t al) noexcept
{
    Zn::Allocators::Delete(p);
};
void operator delete[](void* p, std::size_t n, std::align_val_t al) noexcept
{
    Zn::Allocators::Delete(p);
};
void operator delete(void* p, std::align_val_t al, const std::nothrow_t&) noexcept
{
    Zn::Allocators::Delete(p);
}
void operator delete[](void* p, std::align_val_t al, const std::nothrow_t&) noexcept
{
    Zn::Allocators::Delete(p);
}

void* operator new(std::size_t n, std::align_val_t al) noexcept(false)
{
    return Zn::Allocators::New(n, static_cast<size_t>(al));
}
void* operator new[](std::size_t n, std::align_val_t al) noexcept(false)
{
    return Zn::Allocators::New(n, static_cast<size_t>(al));
}
void* operator new(std::size_t n, std::align_val_t al, const std::nothrow_t&) noexcept
{
    return Zn::Allocators::New(n, static_cast<size_t>(al));
}
void* operator new[](std::size_t n, std::align_val_t al, const std::nothrow_t&) noexcept
{
    return Zn::Allocators::New(n, static_cast<size_t>(al));
}
#endif

#pragma warning(pop)
