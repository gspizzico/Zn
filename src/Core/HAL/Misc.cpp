#include <Corepch.h>
#include "HAL/Misc.h"
#include "HAL/Guid.h"
#include "HAL/PlatformTypes.h"

namespace Zn
{
SystemInfo Misc::GetSystemInfo()
{
    return PlatformMisc::GetSystemInfo();
}

void Misc::Exit(bool bWithErrors)
{
    PlatformMisc::Exit(bWithErrors);
}

Guid Misc::GenerateGuid()
{
    return PlatformMisc::GenerateGuid();
}
} // namespace Zn
