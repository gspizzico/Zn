#pragma once
#include "Core/HAL/BasicTypes.h"

namespace Zn
{
	// Similar to MEMORYSTATUSEX structure.
    struct MemoryStatus
    {
        uint64 m_MemoryLoad = 0;
        uint64 m_TotalPhys = 0;
        uint64 m_AvailPhys = 0;
        uint64 m_TotalPageFile = 0;
        uint64 m_AvailPageFile = 0;
        uint64 m_TotalVirtual = 0;
        uint64 m_AvailVirtual = 0;
        uint64 m_AvailExtendedVirtual = 0;
    };

    enum class StorageUnit : uint64_t
    {
        Byte = 1,
        KiloByte = Byte << 10,
        MegaByte = Byte << 20,
        GigaByte = Byte << 30
    };

    class Memory
    {
    public:

        static MemoryStatus GetMemoryStatus();

		static uintptr_t Align(uintptr_t bytes, size_t alignment);

        static void* Align(void* address, size_t alignment);

		static void* AlignToAddress(void* address, void* start_address, size_t alignment);
        
//		static void* AlignDown(void* address, size_t alignment);

		static bool IsAligned(void* address, size_t alignment);

		static void* AddOffset(void* address, size_t offset);

		static void* SubOffset(void* address, size_t offset);

        static ptrdiff_t GetDistance(const void* first, const void* second);

        // #todo call it memset?
		static void MarkMemory(void* begin, void* end, int8_t pattern);

		static void Memzero(void* begin, void* end);

        static void Memzero(void* begin, size_t size);

        template<typename T>
        static void Memzero(T& data)
        {
            Memzero(&data, sizeof(T));
        }

		static uint64_t Convert(uint64_t size, StorageUnit convert_to, StorageUnit convert_from);
    };

    class MemoryDebug
    {
    public:

        static void MarkUninitialized(void* begin, void* end);

        static void MarkFree(void* begin, void* end);

		static void TrackAllocation(void* address, size_t size);
		
		static void TrackDeallocation(void* address);

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

		bool operator==(const MemoryRange& other) const;

		void* Begin() const { return m_Begin; }

		void* End() const { return m_End; }

		bool Contains(const void* address) const { return Memory::GetDistance(address, m_Begin) >= 0 && Memory::GetDistance(address, m_End) < 0; }
		
		bool Contains(const MemoryRange& other) const { return Memory::GetDistance(other.m_Begin, m_Begin) >= 0 && Memory::GetDistance(m_End, other.m_End) < 0; }

		size_t Size() const { return static_cast<size_t>(Memory::GetDistance(m_End, m_Begin)); }

	private:

		void* m_Begin	= nullptr;

		void* m_End		= nullptr;
	};
}