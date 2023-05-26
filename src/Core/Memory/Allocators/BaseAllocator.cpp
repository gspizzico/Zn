#include <Corepch.h>
#include <Memory/Allocators/BaseAllocator.h>
#include <memory>

using namespace Zn;

void* SystemAllocator::operator new(size_t size)
{
    return malloc(size);
}

void SystemAllocator::operator delete(void* ptr)
{
    free(ptr);
}

void* SystemAllocator::operator new[](size_t size)
{
    return malloc(size);
}

void SystemAllocator::operator delete[](void* ptr)
{
    return free(ptr);
}

Zn::TrackedMalloc::~TrackedMalloc()
{
    for (auto address : allocations)
    {
        ZN_MEMTRACE_FREE(address);

        SystemAllocator::operator delete(address);
    }

    allocations.clear();
}

void* Zn::TrackedMalloc::Malloc(size_t size, size_t alignment)
{
    auto address = SystemAllocator::operator new(size);
    allocations.insert(address);
    return address;
}

bool Zn::TrackedMalloc::Free(void* ptr)
{
    if (ptr && allocations.erase(ptr) > 0)
    {
        SystemAllocator::operator delete(ptr);

        return true;
    }

    return false;
}
