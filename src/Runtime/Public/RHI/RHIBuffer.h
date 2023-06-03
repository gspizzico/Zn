#pragma once

#include <Core/CoreTypes.h>
#include <RHI/RHIResource.h>

namespace Zn
{
enum class RHIBufferUsage : uint32
{
    TransferSrc = 0b1,
    TransferDst = 0b10,
    Uniform     = 0b100,
    Storage     = 0b1000,
    Indirect    = 0b10000,
    Vertex      = 0b100000,
    Index       = 0b100000,
};

struct RHIBuffer
{
    uint32         size;
    RHIBufferUsage bufferUsage;
};

struct RHIBufferDescriptor
{
    uint32           size;
    RHIBufferUsage   bufferUsage;
    RHIResourceUsage memoryUsage;
};
} // namespace Zn
