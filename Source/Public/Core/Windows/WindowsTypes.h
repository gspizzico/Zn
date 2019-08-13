#pragma once

#include "Core/Windows/WindowsMemory.h"
#include "Core/Windows/WindowsMisc.h"

namespace Zn
{
    typedef WindowsMemory PlatformMemory;
    typedef WindowsVirtualMemory PlatformVirtualMemory;
    typedef WindowsMisc PlatformMisc;
}