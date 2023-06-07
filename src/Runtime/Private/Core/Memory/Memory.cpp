#include <Core/Memory/Memory.h>
#include <Core/CoreAssert.h>
#include <Core/CorePlatform.h>
#include <Core/Trace/Trace.h>
#include <Core/Memory/Allocators/BaseAllocator.h>

using namespace Zn;

namespace Zn::Allocators
{
// TODO: Make Thread Safe
BaseAllocator* GDefaultAllocator    = nullptr;
BaseAllocator* GAllocator           = nullptr;
bool           GIsCreatingAllocator = false;

int CreateGAllocatorSafe()
{
    GAllocator = PlatformMemory::CreateAllocator();
    return 1;
}

void CreateGAllocator()
{
    if (GIsCreatingAllocator == false)
    {
        GIsCreatingAllocator      = true;
        static int staticInstance = CreateGAllocatorSafe();
        GIsCreatingAllocator      = false;
    }
    else if (GDefaultAllocator == nullptr)
    {
        GDefaultAllocator = new TrackedMalloc();
    }
    // Thread-safe, but do we need it?
}

void* New(sizet size_, sizet alignment_)
{
    if (GAllocator == NULL)
    {
        CreateGAllocator();
    }

    auto allocator = GIsCreatingAllocator == false ? GAllocator : GDefaultAllocator;

    check(allocator);

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

static_assert(Zn::MemoryAlignment::DefaultAlignment > 0);

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
    return Zn::Allocators::New(n, MemoryAlignment::DefaultAlignment);
}
void* operator new[](std::size_t n) noexcept(false)
{
    return Zn::Allocators::New(n, MemoryAlignment::DefaultAlignment);
}

void* operator new(std::size_t n, const std::nothrow_t& tag) noexcept
{
    (void) (tag);
    return Zn::Allocators::New(n, MemoryAlignment::DefaultAlignment);
}
void* operator new[](std::size_t n, const std::nothrow_t& tag) noexcept
{
    (void) (tag);
    return Zn::Allocators::New(n, MemoryAlignment::DefaultAlignment);
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
