#include <Memory/Memory.h>
#include <CoreAssert.h>
#include <CorePlatform.h>
#include <Trace/Trace.h>

namespace Zn
{
MemoryStatus Memory::GetMemoryStatus()
{
    return PlatformMemory::GetMemoryStatus();
}

uintptr_t Memory::Align(uintptr bytes_, sizet alignment_)
{
    const sizet mask = alignment_ - 1;
    return (bytes_ + mask) & ~mask;
}

void* Memory::Align(void* address_, sizet alignment_)
{
    return reinterpret_cast<void*>(Memory::Align(reinterpret_cast<uintptr_t>(address_), alignment_));
}

void* Memory::AlignToAddress(void* address_, void* startAddress_, sizet alignment_)
{
    auto distance = GetDistance(address_, startAddress_);
    return AddOffset(startAddress_, (distance - distance % alignment_));
}

// void* Memory::AlignDown(void* address_, sizet alignment)
//{
//	return IsAligned(address_, alignment) ? address_ : Memory::SubOffset(Memory::Align(address_, alignment), alignment);
// }

bool Memory::IsAligned(void* address_, sizet alignment_)
{
    return reinterpret_cast<uintptr>(address_) % alignment_ == 0;
}
void* Memory::AddOffset(void* address_, sizet offset_)
{
    return reinterpret_cast<uintptr*>((uintptr) address_ + (uintptr) offset_);
}
void* Memory::SubOffset(void* address_, sizet offset_)
{
    return reinterpret_cast<uintptr*>((uintptr) address_ - (uintptr) offset_);
}
ptrdiff Memory::GetDistance(const void* first_, const void* second_)
{
    return reinterpret_cast<intptr>(first_) - reinterpret_cast<intptr_t>(second_);
}

void Memory::MarkMemory(void* begin_, void* end_, int8_t pattern_)
{
    std::fill(reinterpret_cast<int8_t*>(begin_), reinterpret_cast<int8_t*>(end_), pattern_);
}

void Memory::Memzero(void* begin_, void* end_)
{
    std::memset(begin_, 0, static_cast<sizet>(GetDistance(end_, begin_)));
}

void Memory::Memzero(void* begin_, sizet size_)
{
    std::memset(begin_, 0, size_);
}

uint64 Memory::Convert(uint64 size_, StorageUnit convertTo_, StorageUnit convertFrom_)
{
    return size_ * uint64(convertFrom_) / uint64(convertTo_);
}

void MemoryDebug::MarkUninitialized(void* begin_, void* end_)
{
#if ZN_DEBUG
    Memory::MarkMemory(begin_, end_, kUninitializedMemoryPattern);
#endif
}
void MemoryDebug::MarkFree(void* begin_, void* end_)
{
#if ZN_DEBUG
    Memory::MarkMemory(begin_, end_, kFreeMemoryPattern);
#endif
}
void MemoryDebug::TrackAllocation(void* address_, sizet size)
{
    PlatformMemory::TrackAllocation(address_, size);
}

void MemoryDebug::TrackDeallocation(void* address_)
{
    PlatformMemory::TrackDeallocation(address_);
}

MemoryRange::MemoryRange(MemoryRange&& other_)
    : begin(other_.begin)
    , end(other_.end)
{
    Memory::Memzero(&other_, sizeof(MemoryRange));
}

bool MemoryRange::operator==(const MemoryRange& other_) const
{
    return begin == other_.begin && end == other_.end;
}

bool MemoryRange::Contains(const MemoryRange& other_) const
{
    if (*this == other_)
        return true;

    const auto distanceFromStart = Memory::GetDistance(other_.begin, begin);

    const auto distanceFromEnd = Memory::GetDistance(end, other_.end);

    return (distanceFromStart >= 0 && distanceFromEnd > 0) || (distanceFromStart > 0 && distanceFromEnd >= 0);
}
} // namespace Zn

// Allocators

#include <Memory/Allocators/BaseAllocator.h>

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
    static int staticInstance = CreateGAllocatorSafe();
}

void* New(sizet size_, sizet alignment_)
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

    auto address = allocator->Malloc(size_, alignment_);

    ZN_MEMTRACE_ALLOC(address, size_);

    return address;
}

void Delete(void* address_)
{
    ZN_MEMTRACE_FREE(address_);

    bool success = GAllocator && GAllocator->Free(address_);

    if (!success)
    {
        success = GDefaultAllocator->Free(address_);
    }

    check(success || (address_ == nullptr));
}
} // namespace Zn::Allocators

using namespace Zn::Allocators;

#pragma warning(push)
#pragma warning(disable : 28251) // microsoft vs code analysis warning

static_assert(Zn::MemoryAlignment::kDefaultAlignment > 0);

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
    return Zn::Allocators::New(n, Zn::MemoryAlignment::kDefaultAlignment);
}
void* operator new[](std::size_t n) noexcept(false)
{
    return Zn::Allocators::New(n, Zn::MemoryAlignment::kDefaultAlignment);
}

void* operator new(std::size_t n, const std::nothrow_t& tag) noexcept
{
    (void) (tag);
    return Zn::Allocators::New(n, Zn::MemoryAlignment::kDefaultAlignment);
}
void* operator new[](std::size_t n, const std::nothrow_t& tag) noexcept
{
    (void) (tag);
    return Zn::Allocators::New(n, Zn::MemoryAlignment::kDefaultAlignment);
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
    return Zn::Allocators::New(n, static_cast<sizet>(al));
}
void* operator new[](std::size_t n, std::align_val_t al) noexcept(false)
{
    return Zn::Allocators::New(n, static_cast<sizet>(al));
}
void* operator new(std::size_t n, std::align_val_t al, const std::nothrow_t&) noexcept
{
    return Zn::Allocators::New(n, static_cast<sizet>(al));
}
void* operator new[](std::size_t n, std::align_val_t al, const std::nothrow_t&) noexcept
{
    return Zn::Allocators::New(n, static_cast<sizet>(al));
}
#endif

#pragma warning(pop)
