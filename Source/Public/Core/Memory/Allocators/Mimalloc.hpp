#pragma once

#include <mimalloc.h>
#include <Core/Memory/Allocators/BaseAllocator.h>

namespace Zn
{
class Mimalloc : public BaseAllocator
{
    virtual void* Malloc(size_t size, size_t alignment = DEFAULT_ALIGNMENT)
    {
        return mi_new(size);
    }

    virtual bool Free(void* ptr)
    {
        mi_free(ptr);

        return true;
    }
};
} // namespace Zn
