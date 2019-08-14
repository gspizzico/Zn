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

		static uintptr_t Align(uintptr_t bytes, size_t alignment);

        static void* Align(void* address, size_t alignment);

		static bool IsAligned(void* address, size_t alignment);

		static void* AddOffset(void* address, size_t offset);

		static void* SubOffset(void* address, size_t offset);

        static ptrdiff_t GetDistance(const void* first, const void* second);
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

	struct MemoryRange
	{
	public:

		MemoryRange() = default;

		constexpr MemoryRange(void* begin, void* end) : m_Begin(begin), m_End(end) {};

		MemoryRange(void* begin, size_t size) : m_Begin(begin), m_End(Memory::AddOffset(m_Begin, size)) {}

		MemoryRange(const MemoryRange& other, size_t alignment) : m_Begin(Memory::Align(other.m_Begin, alignment)), m_End(other.m_End) {}

		MemoryRange(const MemoryRange& other) = default;

		MemoryRange& operator=(const MemoryRange&) = default;

		MemoryRange(MemoryRange&& other);

		void* Begin() const { return m_Begin; }

		void* End() const { return m_End; }

		bool Contains(void* address) const { return Memory::GetDistance(address, m_Begin) >= 0 && Memory::GetDistance(address ,m_End) <= 0; }
		
		bool Contains(const MemoryRange& other) const { return Memory::GetDistance(other.m_Begin, m_Begin) >= 0 && Memory::GetDistance(m_End, other.m_End) <= 0; }

		size_t Size() const { return static_cast<size_t>(Memory::GetDistance(m_End, m_Begin)); }

	private:

		void* m_Begin	= nullptr;

		void* m_End		= nullptr;
	};
}