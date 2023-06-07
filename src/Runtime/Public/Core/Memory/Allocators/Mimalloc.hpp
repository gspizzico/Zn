#pragma once

#include <Core/CoreTypes.h>
#include <Core/Memory/Allocators/BaseAllocator.h>
#include <mimalloc.h>

namespace Zn
{
class Mimalloc : public BaseAllocator
{
    virtual void* Malloc(sizet size_, sizet alignment_ = MemoryAlignment::DefaultAlignment)
    {
        return mi_new_aligned(size_, alignment_);
    }

    virtual bool Free(void* ptr_)
    {
        mi_free(ptr_);

        return true;
    }
};
} // namespace Zn
