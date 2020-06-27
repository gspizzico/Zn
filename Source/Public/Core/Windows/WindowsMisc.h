#pragma once
#include "Core/HAL/Misc.h"

namespace Zn
{
	struct Guid;

    class WindowsMisc
    {
    public:
        static SystemInfo GetSystemInfo();

		static void Exit(bool bWithErrors = false);

		static Guid GenerateGuid();

		static uint32 GetLastError();
    };
}
