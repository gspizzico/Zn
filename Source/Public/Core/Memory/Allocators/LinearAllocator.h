#pragma once

#include "Core/Memory/VirtualMemory.h"

namespace Zn
{
    class LinearAllocator
    {
    public:
        
        LinearAllocator() = default;

        LinearAllocator(size_t capacity, size_t alignment);
        
        ~LinearAllocator();

        void* Allocate(size_t size, size_t alignment = 1);

        bool Free();

    private:

		MemoryResource m_Memory;

        void*   m_Address             = nullptr;

        void*   m_NextPageAddress     = nullptr;
    };
}
