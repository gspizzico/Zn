#pragma once

#include <Core/CoreTypes.h>
#include <RHI/RHI.h>

namespace Zn
{

enum class RHIFormat
{
    Undefined,
    RGBA8_UNorm,
    BGRA8_SRGB,
    D32_Float
};

enum class RHITextureFlags : uint32
{
    TransferSrc  = 0b1,
    TransferDst  = 0b10,
    Sampled      = 0b100,
    Storage      = 0b1000,
    Color        = 0b10000,
    DepthStencil = 0b100000,
    Transient    = 0b1000000,
    Input        = 0b10000000,
};

ENABLE_BITMASK_OPERATORS(RHITextureFlags);

struct RHITextureDescriptor
{
    uint32          width;
    uint32          height;
    uint8           numMips;
    uint8           numChannels;
    uint8           channelSize;
    SampleCount     samples;
    RHIFormat       format;
    RHITextureFlags flags;
    void*           data;
    String          id;
    String          debugName;
};

struct RHITexture
{
    uint32          width;
    uint32          height;
    RHIFormat       format;
    RHITextureFlags flags;
};
} // namespace Zn
