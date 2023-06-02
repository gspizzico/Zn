#pragma once

#include <Core/CoreTypes.h>
#include <Core/HAL/Misc.h>
#include <Core/Misc/Guid.h>

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
