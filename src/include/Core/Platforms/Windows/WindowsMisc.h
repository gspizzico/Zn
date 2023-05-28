#pragma once
#include "HAL/Misc.h"

namespace Zn
{
struct Guid;

class WindowsMisc
{
  public:
    static SystemInfo GetSystemInfo();

    static void Exit(bool withErrors_ = false);

    static Guid GenerateGuid();

    static uint32 GetLastError();

    static void LogDebug(cstring message_);

    static void LogConsole(cstring message_);
};
} // namespace Zn
