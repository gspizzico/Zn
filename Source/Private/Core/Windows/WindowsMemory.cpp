#include "Core/Windows/WindowsMemory.h"
#include <windows.h>
#include <sysinfoapi.h>

namespace Zn::Memory
{
    MemoryStatus WindowsMemory::GetMemoryStatus() const
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
}