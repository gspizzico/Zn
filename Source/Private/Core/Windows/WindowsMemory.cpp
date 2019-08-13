#include "Core/Windows/WindowsMemory.h"
#include "Core/Windows/WindowsMisc.h"
#include <windows.h>
#include <memoryapi.h>
#include <sysinfoapi.h>

namespace Zn
{
    MemoryStatus WindowsMemory::GetMemoryStatus()
    {
        _MEMORYSTATUSEX WinMemStatus;
        WinMemStatus.dwLength = sizeof(WinMemStatus);

        GlobalMemoryStatusEx(&WinMemStatus);

        return
        {
            (uint64)WinMemStatus.dwMemoryLoad,
            (uint64)WinMemStatus.ullTotalPhys,              (uint64)WinMemStatus.ullAvailPhys,
            (uint64)WinMemStatus.ullTotalPageFile,          (uint64)WinMemStatus.ullAvailPageFile,
            (uint64)WinMemStatus.ullTotalVirtual,           (uint64)WinMemStatus.ullAvailVirtual,
            (uint64)WinMemStatus.ullAvailExtendedVirtual
        };
    }

    void* WindowsVirtualMemory::Reserve(size_t size)
    {
        return VirtualAlloc(NULL, size, MEM_RESERVE, PAGE_READWRITE);
    }
    void* WindowsVirtualMemory::Allocate(size_t size)
    {
        return VirtualAlloc(NULL, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    }
    bool WindowsVirtualMemory::Release(void * address)
    {
        return VirtualFree(address, 0, MEM_RELEASE);
    }
    bool WindowsVirtualMemory::Commit(void * address, size_t size)
    {
        return VirtualAlloc(address, size, MEM_COMMIT, PAGE_READWRITE);
    }
    bool WindowsVirtualMemory::Decommit(void * address, size_t size)
    {
        return VirtualFree(address, size, MEM_DECOMMIT);
    }
    size_t WindowsVirtualMemory::GetPageSize()
    {
        return WindowsMisc::GetSystemInfo().m_PageSize;
    }
}