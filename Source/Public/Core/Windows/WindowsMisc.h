#pragma once
#include "Core/HAL/Misc.h"

namespace Zn
{
    class WindowsMisc
    {
    public:
        static SystemInfo GetSystemInfo();

		static void Exit(bool bWithErrors = false);
    };
}
