#pragma once

#include <Core/CoreTypes.h>
#include <RHI/RHIDescriptor.h>
#include <RHI/RHITexture.h>
#include <RHI/RHIShader.h>
#include <RHI/RHIResource.h>

namespace Zn::RHI
{
struct VertexInputBinding
{
    enum InputRate
    {
        Vertex,
        Instance
    };

    uint32    binding   = 0;
    uint32    stride    = 0;
    InputRate inputRate = Vertex;
};

struct VertexInputAttribute
{
    uint32    location = 0;
    uint32    binding  = 0;
    RHIFormat format   = RHIFormat::Undefined;
    uint32    offset   = 0;
};

struct VertexInputLayout
{
    Span<const VertexInputBinding>   vertexInputs;
    Span<const VertexInputAttribute> vertexAttributes;
};

enum class PolygonMode : uint8
{
    Fill,
    Line,
    Point
};

enum class CullMode : uint8
{
    None         = 0x0,
    Front        = 0x1,
    Back         = 0x2,
    FrontAndBack = 0x3
};

ENABLE_BITMASK_OPERATORS(CullMode);

struct BlendAttachment
{
    bool           enable              = false;
    BlendFactor    srcColorBlendFactor = BlendFactor::Zero;
    BlendFactor    dstColorBlendFactor = BlendFactor::Zero;
    BlendOp        colorBlendOp        = BlendOp::Add;
    BlendFactor    srcAlphaBlendFactor = BlendFactor::Zero;
    BlendFactor    dstAlphaBlendFactor = BlendFactor::Zero;
    BlendOp        alphaBlendOp        = BlendOp::Add;
    ColorComponent colorWriteMask {};
};

struct ColorBlendState
{
    bool                        enableLogicOp = false;
    LogicOp                     logicOp       = LogicOp::Clear;
    Span<const BlendAttachment> attachments;
};

struct DepthStencilState
{
    bool      enableDepthTest      = false;
    bool      enableDepthWrite     = false;
    CompareOp depthCompareOp       = CompareOp::Never;
    bool      enableDepthBoundTest = false;
    bool      enableStencilTest    = false;
    float     minDepthBounds       = 0.f;
    float     maxDepthBounds       = 0.f;
};

struct PushConstantRange
{
    ShaderStage stageFlags = ShaderStage::All;
    uint32      offset     = 0;
    uint32      size       = 0;
};

struct PipelineDescription
{
    Span<const DescriptorSetLayoutHandle> descriptorSetLayouts;
    Span<const PushConstantRange>         pushConstantRanges {};
    Span<const Shader>                    shaders;
    VertexInputLayout                     vertexInputLayout {};
    PolygonMode                           polygonMode = PolygonMode::Fill;
    CullMode                              cullMode    = CullMode::None;
    ColorBlendState                       colorBlendState {};
    DepthStencilState                     depthStencilState {};
    RenderPassHandle                      renderPass;
    // Dynamic State
};
} // namespace Zn::RHI
