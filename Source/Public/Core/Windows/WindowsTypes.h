#pragma once

#include "Core/Windows/WindowsMemory.h"
#include "Core/Windows/WindowsMisc.h"
#include "Core/Windows/WindowsThreads.h"
#include "Core/Windows/WindowsCriticalSection.h"

namespace Zn
{
    typedef WindowsMemory PlatformMemory;
    typedef WindowsVirtualMemory PlatformVirtualMemory;
    typedef WindowsMisc PlatformMisc;
	typedef WindowsThreads PlatformThreads;

	typedef WindowsCriticalSection CriticalSection;
}