#pragma once
#include "Core/HAL/Misc.h"

namespace Zn
{
struct Guid;

class WindowsMisc
{
  public:
    static SystemInfo GetSystemInfo();

    static void Exit(bool with_errors = false);

    static Guid GenerateGuid();

    static uint32 GetLastError();

    static void LogDebug(cstring message);

    static void LogConsole(cstring message);
};
} // namespace Zn
