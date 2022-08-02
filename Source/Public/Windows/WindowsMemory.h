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

		static void TrackAllocation(void* address, size_t size);

		static void TrackDeallocation(void* address);

        static BaseAllocator* CreateAllocator();
    };

    class WindowsVirtualMemory
    {
    public:
        static void* Reserve(size_t size);

        static void* Allocate(size_t size);

        static bool Release(void* address);

        static bool Commit(void* address, size_t size);

        static bool Decommit(void* address, size_t size);

        static size_t GetPageSize();

		static VirtualMemoryInformation GetMemoryInformation(void* address, size_t size);
    };
}