#pragma once
#include "Core/HAL/PlatformTypes.h"

namespace Zn
{
    namespace Memory
    {
        struct MemoryStatus
        {
            uint64 m_UsedMemory             = 0;
            uint64 m_TotalPhys              = 0;
            uint64 m_AvailPhys              = 0;
            uint64 m_TotalPageFile          = 0;
            uint64 m_AvailPageFile          = 0;
            uint64 m_TotalVirtual           = 0;
            uint64 m_AvailVirtual           = 0;
            uint64 m_AvailExtendedVirtual   = 0;
        };

        enum class StorageUnit
        {
            Byte     = 1,
            KiloByte = 1024,
            MegaByte = KiloByte * KiloByte,
            GigaByte = KiloByte * KiloByte * KiloByte
        };

        class IMemory
        {
        public:

            static void Register(IMemory* memory_handler);

            static const IMemory* Get();

            virtual MemoryStatus GetMemoryStatus() const = 0;

        private:

            static UniquePtr<IMemory> s_Memory;
        };
    }
}