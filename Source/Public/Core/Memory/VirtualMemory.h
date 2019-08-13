#pragma once

namespace Zn
{
    class VirtualMemory
    {
    public:
        static void* Reserve(size_t size);

        static void* Allocate(size_t size);

        static bool Release(void* address);

        static bool Commit(void* address, size_t size);

        static bool Decommit(void* address, size_t size);

        static size_t GetPageSize();

        static size_t AlignToPageSize(size_t size);
    };
}
