#include "Core/HAL/Misc.h"
#include "Core/HAL/PlatformTypes.h"

namespace Zn
{
    SystemInfo Misc::GetSystemInfo()
    {
        return PlatformMisc::GetSystemInfo();
    }
}
