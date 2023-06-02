#include <Windows/Core/WindowsMisc.h>
#include <Windows/Core/WindowsPrivate.h>
#include <Core/Misc/Guid.h>
#include <Core/CoreAssert.h>

namespace Zn
{
SystemInfo WindowsMisc::GetSystemInfo()
{
    _SYSTEM_INFO winSystemInfo;
    GetNativeSystemInfo(&winSystemInfo);

    SystemInfo systemInfo {
        .pageSize              = winSystemInfo.dwPageSize,
        .allocationGranularity = winSystemInfo.dwAllocationGranularity,
        .numOfProcessors       = (uint8) winSystemInfo.dwNumberOfProcessors,
        .architecture          = [&winSystemInfo]() -> ProcessorArchitecture
        {
            if (winSystemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
                return ProcessorArchitecture::x64;
            else if (winSystemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL)
                return ProcessorArchitecture::x86;
            else if (winSystemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM)
                return ProcessorArchitecture::ARM;
            else if (winSystemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64)
                return ProcessorArchitecture::IA64;
            else if (winSystemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM64)
                return ProcessorArchitecture::ARM64;
            else if (winSystemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_UNKNOWN)
                return ProcessorArchitecture::Unknown;

            return ProcessorArchitecture::Unknown;
        }(),
    };

    return systemInfo;
}

void WindowsMisc::Exit(bool withErrors_)
{
    exit(withErrors_ ? EXIT_FAILURE : EXIT_SUCCESS);
}

Guid WindowsMisc::GenerateGuid()
{
    UUID winGuid;
    auto result = ::UuidCreateSequential(&winGuid);

    check(result != RPC_S_UUID_NO_ADDRESS);

    Guid znGuid;
    znGuid.a = winGuid.Data1;
    znGuid.b = winGuid.Data3 | (winGuid.Data2 << sizeof(winGuid.Data2) * 8);

    for (size_t Index = 0; Index < 4; ++Index)
    {
        znGuid.c = (znGuid.c << 8) | winGuid.Data4[Index];
    }

    for (size_t Index = 4; Index < 8; ++Index)
    {
        znGuid.d = (znGuid.d << 8) | winGuid.Data4[Index];
    }

    return znGuid;
}

uint32 WindowsMisc::GetLastError()
{
    return ::GetLastError(); // #TODO Print last error message in a nice way.
}

void WindowsMisc::LogDebug(cstring message_)
{
    OutputDebugString(message_);
}
void WindowsMisc::LogConsole(cstring message_)
{
    if (HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE); consoleHandle != NULL)
    {
        WriteConsole(consoleHandle, message_, static_cast<DWORD>(strlen(message_)), nullptr, nullptr);
    }
}
} // namespace Zn
