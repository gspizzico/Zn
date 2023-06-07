#pragma once

#include <Core/CoreTypes.h>

namespace Zn
{
// Similar to MEMORYSTATUSEX structure.
struct MemoryStatus
{
    uint64 memoryLoad           = 0;
    uint64 totalPhys            = 0;
    uint64 availPhys            = 0;
    uint64 totalPageFile        = 0;
    uint64 availPageFile        = 0;
    uint64 totalVirtual         = 0;
    uint64 availVirtual         = 0;
    uint64 availExtendedVirtual = 0;
};

// Namespace used for new / delete overrides.
namespace Allocators
{
void* New(size_t size_, size_t alignment_);

void Delete(void* address_);
} // namespace Allocators

} // namespace Zn
