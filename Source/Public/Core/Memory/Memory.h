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

        static void* Align(void* address, size_t alignment);

        static bool IsAligned(void* address, size_t alignment);

        static void* AddOffset(void* address, size_t offset);

        static void* SubOffset(void* address, size_t offset);

        static ptrdiff_t GetOffset(const void* first, const void* second);
    };

    class MemoryDebug
    {
    public:

		static void MarkMemory(void* begin, void* end, int8_t pattern);

        static void MarkUninitialized(void* begin, void* end);

        static void MarkFree(void* begin, void* end);

    private:

        static constexpr int8_t kUninitializedMemoryPattern = 0x5C;
        
        static constexpr int8_t kFreeMemoryPattern          = 0x5F;
    };
}