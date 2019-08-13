#include "Core/Windows/WindowsMisc.h"
#include <windows.h>

namespace Zn
{
    SystemInfo WindowsMisc::GetSystemInfo()
    {
        _SYSTEM_INFO WinSystemInfo;
        GetNativeSystemInfo(&WinSystemInfo);

        SystemInfo SystemInfo;
        SystemInfo.m_PageSize = WinSystemInfo.dwPageSize;
        SystemInfo.m_AllocationGranularity = WinSystemInfo.dwAllocationGranularity;
        SystemInfo.m_NumOfProcessors = WinSystemInfo.dwNumberOfProcessors;
        SystemInfo.m_Architecture = 
            [&WinSystemInfo]() ->ProcessorArchitecture
        {
            if (WinSystemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
                return ProcessorArchitecture::x64;
            else if (WinSystemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL)
                return ProcessorArchitecture::x86;
            else if (WinSystemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM)
                return ProcessorArchitecture::ARM;
            else if (WinSystemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64)
                return ProcessorArchitecture::IA64;
            else if (WinSystemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM64)
                return ProcessorArchitecture::ARM64;
            else if (WinSystemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_UNKNOWN)
                return ProcessorArchitecture::Unknown;

            return ProcessorArchitecture::Unknown;
        }();

        return SystemInfo;
    }
}
