#include <Core/Corepch.h>
#include "Windows/WindowsMemory.h"
#include "Windows/WindowsMisc.h"
#include "Windows/WindowsCommon.h"
#include "Core/Build.h"

#include <Core/Memory/Allocators/Mimalloc.hpp>

#define ZN_WINDOWS_TRACK_MEMORY (ZN_TRACK_MEMORY && !ZN_RELEASE) && 0

#if ZN_WINDOWS_TRACK_MEMORY
    #include <VSCustomNativeHeapEtwProvider.h>
#endif

namespace Zn
{
MemoryStatus WindowsMemory::GetMemoryStatus()
{
    _MEMORYSTATUSEX WinMemStatus;
    WinMemStatus.dwLength = sizeof(WinMemStatus);

    GlobalMemoryStatusEx(&WinMemStatus);

    return {(uint64) WinMemStatus.dwMemoryLoad,
            (uint64) WinMemStatus.ullTotalPhys,
            (uint64) WinMemStatus.ullAvailPhys,
            (uint64) WinMemStatus.ullTotalPageFile,
            (uint64) WinMemStatus.ullAvailPageFile,
            (uint64) WinMemStatus.ullTotalVirtual,
            (uint64) WinMemStatus.ullAvailVirtual,
            (uint64) WinMemStatus.ullAvailExtendedVirtual};
}
#if ZN_WINDOWS_TRACK_MEMORY
auto GHeapTracker = std::make_unique<VSHeapTracker::CHeapTracker>("Zn::WindowsMemory");
#endif

void WindowsMemory::TrackAllocation(void* address_, sizet size_)
{
#if ZN_WINDOWS_TRACK_MEMORY
    GHeapTracker->AllocateEvent(address_, (unsigned long) size_);
#endif
}

void WindowsMemory::TrackDeallocation(void* address_)
{
#if ZN_WINDOWS_TRACK_MEMORY
    GHeapTracker->DeallocateEvent(address_);
#endif
}

BaseAllocator* WindowsMemory::CreateAllocator()
{
    return new Mimalloc();
}

void* WindowsVirtualMemory::Reserve(sizet size_)
{
    return VirtualAlloc(NULL, size_, MEM_RESERVE, PAGE_READWRITE);
}

void* WindowsVirtualMemory::Allocate(sizet size_)
{
    return VirtualAlloc(NULL, size_, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
}

bool WindowsVirtualMemory::Release(void* address_)
{
    return VirtualFree(address_, 0, MEM_RELEASE);
}

bool WindowsVirtualMemory::Commit(void* address_, sizet size_)
{
    return VirtualAlloc(address_, size_, MEM_COMMIT, PAGE_READWRITE);
}

#pragma warning(push)
#pragma warning(disable : 6250) // Warning C6250 Calling 'VirtualFree' without the MEM_RELEASE flag might free memory but not address
                                // descriptors(VADs).This causes address space leaks.

bool WindowsVirtualMemory::Decommit(void* address_, sizet size_)
{
    return VirtualFree(address_, size_, MEM_DECOMMIT);
}

#pragma warning(pop)

size_t WindowsVirtualMemory::GetPageSize()
{
    return WindowsMisc::GetSystemInfo().pageSize;
}

VirtualMemoryInformation WindowsVirtualMemory::GetMemoryInformation(void* address_, sizet size_)
{
    MEMORY_BASIC_INFORMATION winMemoryInformation;
    auto                     result = VirtualQuery(address_, &winMemoryInformation, sizeof(winMemoryInformation));

    check(result > 0 && result == sizeof(winMemoryInformation));

    VirtualMemoryInformation memoryInformation;

    if (winMemoryInformation.State == MEM_RESERVE)
    {
        memoryInformation.state = VirtualMemory::State::kReserved;
    }
    else if (winMemoryInformation.State == MEM_FREE)
    {
        memoryInformation.state = VirtualMemory::State::kFree;

        memoryInformation.range = MemoryRange((void*) winMemoryInformation.BaseAddress, 0ull);

        return memoryInformation;
    }
    else if (winMemoryInformation.State == MEM_COMMIT)
    {
        memoryInformation.state = VirtualMemory::State::kCommitted;
    }

    memoryInformation.range = MemoryRange((void*) winMemoryInformation.AllocationBase, winMemoryInformation.RegionSize);

    return memoryInformation;
}
} // namespace Zn
