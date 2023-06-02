#pragma once

#include <Core/CoreTypes.h>
#include <Core/Memory/Memory.h>

namespace Zn
{

class SystemAllocator
{
  public:
    virtual ~SystemAllocator() = default;

    void* operator new(size_t size_);

    void operator delete(void* ptr_);

    void* operator new[](size_t size_);

    void operator delete[](void* ptr_);
};

class BaseAllocator : public SystemAllocator
{
  public:
    virtual ~BaseAllocator() = default;

    virtual void* Malloc(size_t size_, size_t alignment_ = MemoryAlignment::kDefaultAlignment) = 0;

    virtual bool Free(void* ptr_) = 0;

    // virtual void* Realloc(void* ptr, size_t size, size_t alignment = DEFAULT_ALIGNMENT) = 0;
};

class TrackedMalloc : public BaseAllocator
{
  public:
    virtual ~TrackedMalloc();

    virtual void* Malloc(sizet size_, sizet alignment_ = MemoryAlignment::kDefaultAlignment) override;

    virtual bool Free(void* ptr_) override;

  private:
    UnorderedSet<void*> allocations;
};
} // namespace Zn
