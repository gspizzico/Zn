#include <Core/Memory/Allocators/BaseAllocator.h>
#include <Core/Trace/Trace.h>
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

void* Zn::TrackedMalloc::Malloc(sizet size_, sizet alignment_)
{
    // TODO: Align
    auto address = SystemAllocator::operator new(size_);
    allocations.insert(address);
    return address;
}

bool Zn::TrackedMalloc::Free(void* ptr_)
{
    if (ptr_ && allocations.erase(ptr_) > 0)
    {
        SystemAllocator::operator delete(ptr_);

        return true;
    }

    return false;
}
