#pragma once

#include <Core/CoreTypes.h>

namespace Zn
{
enum class ImageLayout : uint8
{
    Undefined,
    ColorAttachment,
    DepthStencilAttachment,
    Present
};

enum class SampleCount : uint8
{
    s1  = 1,
    s2  = 2,
    s4  = 4,
    s8  = 8,
    s16 = 16,
    s32 = 32,
    s64 = 64
};

enum class PipelineType : uint8
{
    Undefined,
    Graphics,
    Compute,
    Raytracing
};

enum class PipelineStage : uint32
{
    None                         = 0,
    TopOfPipe                    = 0x1,
    DrawIndirect                 = 0x2,
    VertexInput                  = 0x4,
    VertexShader                 = 0x8,
    TessellationControlShader    = 0x10,
    TessellationEvaluationShader = 0x20,
    GeometryShader               = 0x40,
    FragmentShader               = 0x80,
    EarlyFragmentTests           = 0x100,
    LateFragmentTests            = 0x200,
    ColorAttachmentOutput        = 0x400,
    ComputeShader                = 0x800,
    Transfer                     = 0x1000,
    BottomOfPipe                 = 0x2000,
    Host                         = 0x4000,
    AllGraphics                  = 0x8000,
    AllCommands                  = 0x10000,
    // TODO: Some stages are missing from VK.
    RayTracingShader             = 0x200000,
    TaskShader                   = 0x80000,
    MeshShader                   = 0x100000,
};

ENABLE_BITMASK_OPERATORS(PipelineStage);

enum class AccessFlag
{
    None                        = 0,
    IndirectCommandRead         = 0x1,
    IndexRead                   = 0x2,
    VertexAttributeRead         = 0x4,
    UniformRead                 = 0x8,
    InputAttachmentRead         = 0x10,
    ShaderRead                  = 0x20,
    ShaderWrite                 = 0x40,
    ColorAttachmentRead         = 0x80,
    ColorAttachmentWrite        = 0x100,
    DepthStencilAttachmentRead  = 0x200,
    DepthStencilAttachmentWrite = 0x400,
    TransferRead                = 0x800,
    TransferWrite               = 0x1000,
    HostRead                    = 0x2000,
    HostWrite                   = 0x4000,
    MemoryRead                  = 0x8000,
    MemoryWrite                 = 0x10000,
    // TODO: Missing VK EXT
};

ENABLE_BITMASK_OPERATORS(AccessFlag);

} // namespace Zn
