#pragma once

#include <CoreTypes.h>

namespace Zn
{
struct Guid;

enum class ProcessorArchitecture
{
    x64,
    x86,
    ARM,
    IA64,
    ARM64,
    Unknown
};

struct SystemInfo
{
    uint64                pageSize;
    uint64                allocationGranularity;
    uint8                 numOfProcessors;
    ProcessorArchitecture architecture;
};
} // namespace Zn
