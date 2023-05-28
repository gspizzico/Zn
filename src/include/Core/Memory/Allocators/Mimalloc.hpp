#pragma once

#include <mimalloc.h>
#include <Memory/Allocators/BaseAllocator.h>

namespace Zn
{
class Mimalloc : public BaseAllocator
{
    virtual void* Malloc(sizet size_, sizet alignment_ = MemoryAlignment::kDefaultAlignment)
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
