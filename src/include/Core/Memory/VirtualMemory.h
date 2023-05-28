#pragma once

#include "Memory/Memory.h"
#include "Log/Log.h"
#include "Log/LogMacros.h"

DECLARE_LOG_CATEGORY(LogMemory);

#if ZN_DEBUG
    #define ZN_VM_CHECK(FunctionCall)                                                                                                      \
        if (!FunctionCall)                                                                                                                 \
        {                                                                                                                                  \
            ZN_LOG(LogMemory, ELogVerbosity::Error, #FunctionCall);                                                                        \
            check(false);                                                                                                                  \
        }
#else
    #define ZN_VM_CHECK(FunctionCall) FunctionCall
#endif

namespace Zn
{
struct VirtualMemoryInformation;

class VirtualMemory
{
  public:
    enum class State
    {
        kReserved = 0, // Indicates reserved pages where a range of the process's virtual address space is reserved without any physical
                       // storage being allocated.
        kCommitted =
            1, // Indicates committed pages for which physical storage has been allocated, either in memory or in the paging file on disk.
        kFree = 2 // Indicates free pages not accessible to the calling process and available to be allocated.
    };

    static void* Reserve(sizet size_);

    static void* Allocate(sizet size_);

    static bool Release(void* address_);

    static bool Commit(void* address_, sizet size_);

    static bool Decommit(void* address_, sizet size_);

    static size_t GetPageSize();

    static size_t AlignToPageSize(sizet size);

    static VirtualMemoryInformation GetMemoryInformation(void* address_, sizet size_);

    static VirtualMemoryInformation GetMemoryInformation(MemoryRange range_);
};

struct VirtualMemoryInformation
{
    MemoryRange range;

    VirtualMemory::State state; // Note: State::kFree -> m_Range is undefined.
};

struct VirtualMemoryRegion
{
  public:
    VirtualMemoryRegion() = default;

    VirtualMemoryRegion(sizet capacity_);

    VirtualMemoryRegion(VirtualMemoryRegion&& other_) noexcept;

    VirtualMemoryRegion(MemoryRange range_) noexcept;

    ~VirtualMemoryRegion();

    VirtualMemoryRegion(const VirtualMemoryRegion&) = delete;

    VirtualMemoryRegion& operator=(const VirtualMemoryRegion&) = delete;

    operator bool() const
    {
        return range.begin != nullptr;
    }

    size_t Size() const
    {
        return range.Size();
    }

    void* Begin() const
    {
        return range.begin;
    }

    void* End() const
    {
        return range.end;
    }

    const MemoryRange& Range() const
    {
        return range;
    }

  private:
    MemoryRange range;
};
} // namespace Zn
