#pragma once

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

        bool IsValidAddress(void* address);

        void*   m_Address             = nullptr;

        void*   m_NextPageAddress     = nullptr;

        void*   m_BaseAddress         = nullptr;

        void*   m_LastAddress         = nullptr;

        size_t  m_MaxAllocatableSize  = 0;
    };
}
