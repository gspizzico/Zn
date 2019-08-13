#include "Core/Memory/Memory.h"
#include "Core/HAL/PlatformTypes.h"
#include "Core/Math/Math.h"

namespace Zn
{
    MemoryStatus Memory::GetMemoryStatus()
    {
        return PlatformMemory::GetMemoryStatus();
    }

    void* Memory::Align(void * address, size_t alignment)
    {
        return reinterpret_cast<void*>(Math::Ceil(reinterpret_cast<uintptr_t>(address), alignment));
    }
    bool Memory::IsAligned(void * address, size_t alignment)
    {
        return reinterpret_cast<uintptr_t>(address) % alignment == 0;
    }
    void* Memory::AddOffset(void* address, size_t offset)
    {
        return reinterpret_cast<int8_t*>(address) + offset;
    }
    void * Memory::SubOffset(void * address, size_t offset)
    {
        return reinterpret_cast<int8_t*>(address) - offset;
    }
    ptrdiff_t Memory::GetOffset(const void * first, const void * second)
    {
        return reinterpret_cast<intptr_t>(first) - reinterpret_cast<intptr_t>(second);
    }

    void MemoryDebug::MarkUninitialized(void * begin, void * end)
    {
        std::fill(
            reinterpret_cast<int8_t*>(begin),
            reinterpret_cast<int8_t*>(end),
            kUninitializedMemoryPattern);
    }
    void MemoryDebug::MarkFree(void * begin, void * end)
    {
        std::fill(
            reinterpret_cast<int8_t*>(begin),
            reinterpret_cast<int8_t*>(end),
            kFreeMemoryPattern);
    }
}

void* operator new(size_t size)
{
    return malloc(size);
}

void* operator new[](size_t size)
{
    return malloc(size);
}

void operator delete (void* mem)
{
    free(mem);
}

void operator delete[](void* mem)
{
    free(mem);
}
