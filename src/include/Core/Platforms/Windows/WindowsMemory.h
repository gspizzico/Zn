#pragma once
#include "Core/Memory/Memory.h"
#include "Core/Memory/VirtualMemory.h"

namespace Zn
{
class BaseAllocator;

class WindowsMemory
{
  public:
    static MemoryStatus GetMemoryStatus();

    static void TrackAllocation(void* address_, sizet size_);

    static void TrackDeallocation(void* address_);

    static BaseAllocator* CreateAllocator();
};

class WindowsVirtualMemory
{
  public:
    static void* Reserve(sizet size_);

    static void* Allocate(sizet size_);

    static bool Release(void* address_);

    static bool Commit(void* address_, sizet size_);

    static bool Decommit(void* address_, sizet size_);

    static sizet GetPageSize();

    static VirtualMemoryInformation GetMemoryInformation(void* address_, sizet size_);
};
} // namespace Zn
