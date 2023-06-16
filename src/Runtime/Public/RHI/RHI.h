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

enum class ShaderStage : uint32
{
    Vertex                 = 0x1,
    TessellationControl    = 0x2,
    TessellationEvaluation = 0x4,
    Geometry               = 0x8,
    Fragment               = 0x10,
    Compute                = 0x20,
    Task                   = 0x40,
    Mesh                   = 0x80,
    AllGraphics            = 0x1F,
    Raygen                 = 0x100,
    AnyHit                 = 0x200,
    ClosestHit             = 0x400,
    Miss                   = 0x800,
    Intersection           = 0x1000,
    Callable               = 0x2000,
    All                    = 0x7FFFFFFF,
};

ENABLE_BITMASK_OPERATORS(ShaderStage);

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

enum class ColorComponent : uint8
{
    R = 0x1,
    G = 0x2,
    B = 0x4,
    A = 0x8
};

ENABLE_BITMASK_OPERATORS(ColorComponent);

enum class BlendFactor : uint32
{
    Zero,
    One,
    SrcColor,
    OneMinusSrcColor,
    DstColor,
    OneMinusDstColor,
    SrcAlpha,
    OneMinusSrcAlpha,
    DstAlpha,
    OneMinusDstAlpha,
    ConstantColor,
    OneMinusConstantColor,
    ConstantAlpha,
    OneMinusConstantAlpha,
    SrcAlphaSaturate,
    Src1Color,
    OneMinusSrc1Color,
    Src1Alpha,
    OneMinusSrc1Alpha,
};

enum class BlendOp
{
    Add,
    Subtract,
    ReverseSubtract,
    Min,
    Max,
};

enum class LogicOp
{
    Clear,
    And,
    AndReverse,
    Copy,
    AndInverted,
    NoOp,
    Xor,
    Or,
    Nor,
    Equivalent,
    Invert,
    OrReverse,
    CopyInverted,
    OrInverted,
    Nand,
    Set,
};

enum class CompareOp
{
    Never,
    Less,
    Equal,
    LessOrEqual,
    Greater,
    NotEqual,
    GreaterOrEqual,
    Always,
};

} // namespace Zn
