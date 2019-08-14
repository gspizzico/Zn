#pragma once
#include "Core/Memory/Memory.h"

namespace Zn
{
    class WindowsMemory
    {
    public:
        static MemoryStatus GetMemoryStatus();
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
    };
}