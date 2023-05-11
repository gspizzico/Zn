#pragma once

#include <Core/Containers/Set.h>

namespace Zn
{
enum
{
    DEFAULT_ALIGNMENT = 0,
    MIN_ALIGNMENT     = 8
};

class SystemAllocator
{
  public:
    virtual ~SystemAllocator() = default;

    void* operator new(size_t size);

    void operator delete(void* ptr);

    void* operator new[](size_t size);

    void operator delete[](void* ptr);
};

class BaseAllocator : public SystemAllocator
{
  public:
    virtual ~BaseAllocator() = default;

    virtual void* Malloc(size_t size, size_t alignment = DEFAULT_ALIGNMENT) = 0;

    virtual bool Free(void* ptr) = 0;

    // virtual void* Realloc(void* ptr, size_t size, size_t alignment = DEFAULT_ALIGNMENT) = 0;
};

class TrackedMalloc : public BaseAllocator
{
  public:
    virtual ~TrackedMalloc();

    virtual void* Malloc(size_t size, size_t alignment = DEFAULT_ALIGNMENT) override;

    virtual bool Free(void* ptr) override;

  private:
    UnorderedSet<void*> allocations;
};
} // namespace Zn
