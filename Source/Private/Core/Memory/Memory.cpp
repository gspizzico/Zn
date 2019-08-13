#include "Core/Memory/Memory.h"
#include "Core/HAL/PlatformTypes.h"

namespace Zn
{
    MemoryStatus Memory::GetMemoryStatus()
    {
        return PlatformMemory::GetMemoryStatus();
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
