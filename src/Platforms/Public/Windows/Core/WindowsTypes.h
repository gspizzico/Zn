#pragma once

#include "Windows/Core/WindowsThreads.h"
#include "Windows/Core/WindowsCriticalSection.h"
#include "Windows/Core/WindowsMemory.h"
#include "Windows/Core/WindowsMisc.h"

namespace Zn
{
typedef WindowsMemory        PlatformMemory;
typedef WindowsVirtualMemory PlatformVirtualMemory;
typedef WindowsMisc          PlatformMisc;
typedef WindowsThreads       PlatformThreads;

typedef WindowsCriticalSection CriticalSection;
} // namespace Zn
