#pragma once

#include "Core/Memory/Memory.h"

namespace Zn::Memory
{
    class WindowsMemory : public IMemory
    {
        virtual MemoryStatus GetMemoryStatus() const override;
    };
}