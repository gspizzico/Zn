#pragma once

#include "Core/Memory/VirtualMemory.h"

namespace Zn
{
    class LinearAllocator
    {
    public:
        
        LinearAllocator() = default;

        LinearAllocator(size_t capacity);
        
        ~LinearAllocator();

        void* Allocate(size_t size, size_t alignment = 1);
        
        bool Free();

		bool IsAllocated(void* address) const;

		const VirtualMemoryRegion& GetMemory() const { return m_Memory; }

		size_t GetAllocatedMemory() const;

		size_t GetRemainingMemory() const;

    private:

		VirtualMemoryRegion m_Memory;

        void*   m_Address             = nullptr;

        void*   m_NextPageAddress     = nullptr;
    };
}
