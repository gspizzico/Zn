#pragma once
#include "Core/HAL/BasicTypes.h"

namespace Zn
{
    struct MemoryStatus
    {
        uint64 m_UsedMemory = 0;
        uint64 m_TotalPhys = 0;
        uint64 m_AvailPhys = 0;
        uint64 m_TotalPageFile = 0;
        uint64 m_AvailPageFile = 0;
        uint64 m_TotalVirtual = 0;
        uint64 m_AvailVirtual = 0;
        uint64 m_AvailExtendedVirtual = 0;
    };

    enum class StorageUnit
    {
        Byte = 1,
        KiloByte = 1024,
        MegaByte = KiloByte * KiloByte,
        GigaByte = KiloByte * KiloByte * KiloByte
    };

    class Memory
    {
    public:

        static MemoryStatus GetMemoryStatus();
    };
}