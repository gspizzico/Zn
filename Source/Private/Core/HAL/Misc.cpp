#include <Znpch.h>
#include "Core/HAL/Misc.h"
#include "Core/HAL/Guid.h"
#include "Core/HAL/PlatformTypes.h"

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
}
