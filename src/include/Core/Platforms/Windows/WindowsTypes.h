#pragma once

#include "Windows/WindowsThreads.h"
#include "Windows/WindowsCriticalSection.h"
#include "Windows/WindowsMemory.h"
#include "Windows/WindowsMisc.h"

namespace Zn
{
typedef WindowsMemory        PlatformMemory;
typedef WindowsVirtualMemory PlatformVirtualMemory;
typedef WindowsMisc          PlatformMisc;
typedef WindowsThreads       PlatformThreads;

typedef WindowsCriticalSection CriticalSection;
} // namespace Zn
