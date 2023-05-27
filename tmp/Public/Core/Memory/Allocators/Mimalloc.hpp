#pragma once

#include <mimalloc.h>
#include <Core/Memory/Allocators/BaseAllocator.h>

namespace Zn
{
class Mimalloc : public BaseAllocator
{
    virtual void* Malloc(size_t size, size_t alignment = MemoryAlignment::kDefaultAlignment)
    {
        return mi_new_aligned(size, alignment);
    }

    virtual bool Free(void* ptr)
    {
        mi_free(ptr);

        return true;
    }
};
} // namespace Zn
