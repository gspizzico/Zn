#include "Core/Memory/Memory.h"

namespace Zn::Memory
{
    UniquePtr<IMemory> IMemory::s_Memory = nullptr;

    void IMemory::Register(IMemory * memory_handler)
    {
        _ASSERT(!s_Memory);
        s_Memory = std::unique_ptr<IMemory>(memory_handler);
    }

    const IMemory* IMemory::Get()
    {
        _ASSERT(s_Memory);
        return s_Memory.get();
    }
}
