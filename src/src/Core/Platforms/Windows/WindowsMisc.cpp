#include <Core/Corepch.h>
#include "Windows/WindowsMisc.h"
#include "Windows/WindowsCommon.h"
#include "Core/Guid.h"

namespace Zn
{
SystemInfo WindowsMisc::GetSystemInfo()
{
    _SYSTEM_INFO WinSystemInfo;
    GetNativeSystemInfo(&WinSystemInfo);

    SystemInfo SystemInfo;
    SystemInfo.pageSize              = WinSystemInfo.dwPageSize;
    SystemInfo.allocationGranularity = WinSystemInfo.dwAllocationGranularity;
    SystemInfo.numOfProcessors       = (uint8) WinSystemInfo.dwNumberOfProcessors;
    SystemInfo.architecture          = [&WinSystemInfo]() -> ProcessorArchitecture
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

    check(Result != RPC_S_UUID_NO_ADDRESS);

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
    return ::GetLastError(); // #TODO Print last error message in a nice way.
}

void WindowsMisc::LogDebug(cstring message)
{
    OutputDebugString(message);
}
void WindowsMisc::LogConsole(cstring message)
{
    if (HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE); consoleHandle != NULL)
    {
        WriteConsole(consoleHandle, message, static_cast<DWORD>(strlen(message)), nullptr, nullptr);
    }
}
} // namespace Zn
