#pragma once

#include "Memory/Memory.h"
#include "Log/Log.h"
#include "Log/LogMacros.h"
#include "Types.h"

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

    static void* Reserve(size_t size);

    static void* Allocate(size_t size);

    static bool Release(void* address);

    static bool Commit(void* address, size_t size);

    static bool Decommit(void* address, size_t size);

    static size_t GetPageSize();

    static size_t AlignToPageSize(size_t size);

    static VirtualMemoryInformation GetMemoryInformation(void* address, size_t size);

    static VirtualMemoryInformation GetMemoryInformation(MemoryRange range);
};

struct VirtualMemoryInformation
{
    MemoryRange m_Range;

    VirtualMemory::State m_State; // Note: State::kFree -> m_Range is undefined.
};

struct VirtualMemoryRegion
{
  public:
    VirtualMemoryRegion() = default;

    VirtualMemoryRegion(size_t capacity);

    VirtualMemoryRegion(VirtualMemoryRegion&& other) noexcept;

    VirtualMemoryRegion(MemoryRange range) noexcept;

    ~VirtualMemoryRegion();

    VirtualMemoryRegion(const VirtualMemoryRegion&) = delete;

    VirtualMemoryRegion& operator=(const VirtualMemoryRegion&) = delete;

    operator bool() const
    {
        return m_Range.Begin() != nullptr;
    }

    size_t Size() const
    {
        return m_Range.Size();
    }

    void* Begin() const
    {
        return m_Range.Begin();
    }

    void* End() const
    {
        return m_Range.End();
    }

    const MemoryRange& Range() const
    {
        return m_Range;
    }

  private:
    MemoryRange m_Range;
};
} // namespace Zn
