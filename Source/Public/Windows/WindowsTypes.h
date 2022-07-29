#pragma once

#include "Windows/WindowsMemory.h"
#include "Windows/WindowsMisc.h"

namespace Zn
{
    typedef WindowsMemory PlatformMemory;
    typedef WindowsVirtualMemory PlatformVirtualMemory;
    typedef WindowsMisc PlatformMisc;
}