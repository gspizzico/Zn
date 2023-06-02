#pragma once

#include <Core/CoreTypes.h>

namespace Zn
{
// Similar to MEMORYSTATUSEX structure.
struct MemoryStatus
{
    uint64 memoryLoad           = 0;
    uint64 totalPhys            = 0;
    uint64 availPhys            = 0;
    uint64 totalPageFile        = 0;
    uint64 availPageFile        = 0;
    uint64 totalVirtual         = 0;
    uint64 availVirtual         = 0;
    uint64 availExtendedVirtual = 0;
};

enum class StorageUnit : uint64
{
    Byte     = 1,
    KiloByte = Byte << 10,
    MegaByte = Byte << 20,
    GigaByte = Byte << 30
};

enum MemoryAlignment
{
    kMinAlignment     = sizeof(std::max_align_t),
    kDefaultAlignment = __STDCPP_DEFAULT_NEW_ALIGNMENT__,
};

class Memory
{
  public:
    static MemoryStatus GetMemoryStatus();

    static uintptr_t Align(uintptr bytes_, sizet alignment_);

    static void* Align(void* address_, sizet alignment_);

    static void* AlignToAddress(void* address_, void* startAddress_, sizet alignment_);

    //		static void* AlignDown(void* address, size_t alignment);

    static bool IsAligned(void* address_, sizet alignment_);

    static void* AddOffset(void* address_, sizet offset_);

    static void* SubOffset(void* address_, sizet offset_);

    static ptrdiff_t GetDistance(const void* first_, const void* second_);

    // #todo call it memset?
    static void MarkMemory(void* begin_, void* end_, int8_t pattern_);

    static void Memzero(void* begin_, void* end_);

    static void Memzero(void* begin_, sizet size_);

    template<typename T>
    static void Memzero(T& data_)
    {
        Memzero(&data_, sizeof(T));
    }

    static uint64_t Convert(uint64 size_, StorageUnit convertTo_, StorageUnit convertFrom_);
};

class MemoryDebug
{
  public:
    static void MarkUninitialized(void* begin_, void* end_);

    static void MarkFree(void* begin_, void* end_);

    static void TrackAllocation(void* address_, sizet size_);

    static void TrackDeallocation(void* address_);

    static constexpr int8_t kUninitializedMemoryPattern = 0x5C;

    static constexpr int8_t kFreeMemoryPattern = 0x5F;
};

struct MemoryRange
{
  public:
    MemoryRange() = default;

    constexpr MemoryRange(void* begin_, void* end_)
        : begin(begin_)
        , end(end_) {};

    MemoryRange(void* begin_, sizet size_)
        : begin(begin_)
        , end(Memory::AddOffset(begin_, size_))
    {
    }

    MemoryRange(const MemoryRange& other_, sizet alignment_)
        : begin(Memory::Align(other_.begin, alignment_))
        , end(other_.end)
    {
    }

    MemoryRange(const MemoryRange& other_) = default;

    MemoryRange& operator=(const MemoryRange& other_) = default;

    MemoryRange(MemoryRange&& other_);

    bool operator==(const MemoryRange& other_) const;

    bool Contains(const void* address_) const
    {
        return Memory::GetDistance(address_, begin) >= 0 && Memory::GetDistance(address_, end) < 0;
    }

    bool Contains(const MemoryRange& other_) const;

    sizet Size() const
    {
        return static_cast<size_t>(Memory::GetDistance(end, begin));
    }

    void* begin = nullptr;

    void* end = nullptr;
};

// Namespace used for new / delete overrides.
namespace Allocators
{
void* New(size_t size_, size_t alignment_);

void Delete(void* address_);
} // namespace Allocators

} // namespace Zn
