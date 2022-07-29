#include "Windows/WindowsMisc.h"
#include "Windows/WindowsCommon.h"
#include "Core/HAL/Guid.h"

namespace Zn
{
    SystemInfo WindowsMisc::GetSystemInfo()
    {
        _SYSTEM_INFO WinSystemInfo;
        GetNativeSystemInfo(&WinSystemInfo);

        SystemInfo SystemInfo;
        SystemInfo.m_PageSize = WinSystemInfo.dwPageSize;
        SystemInfo.m_AllocationGranularity = WinSystemInfo.dwAllocationGranularity;
        SystemInfo.m_NumOfProcessors = (uint8) WinSystemInfo.dwNumberOfProcessors;
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

	void WindowsMisc::Exit(bool bWithErrors)
	{
		exit(bWithErrors ? EXIT_FAILURE : EXIT_SUCCESS);
	}

	Guid WindowsMisc::GenerateGuid()
	{
		UUID WindowsGuid;
		auto Result = UuidCreateSequential(&WindowsGuid);

		_ASSERT(Result != RPC_S_UUID_NO_ADDRESS);

		Guid ZnGuid;
		ZnGuid.A = WindowsGuid.Data1;
		ZnGuid.B = WindowsGuid.Data3 | (WindowsGuid.Data2 << sizeof(WindowsGuid.Data2) * 8);
		
		for (size_t Index = 0; Index < 4; ++Index)
		{
			ZnGuid.C = (ZnGuid.C << 8) | WindowsGuid.Data4[Index];
		}

		for (size_t Index = 4; Index < 8; ++Index)
		{
			ZnGuid.D = (ZnGuid.D << 8) | WindowsGuid.Data4[Index];
		}

		return ZnGuid;
	}
	
	uint32 WindowsMisc::GetLastError()
	{
		return ::GetLastError(); //#TODO Print last error message in a nice way.
	}
}
