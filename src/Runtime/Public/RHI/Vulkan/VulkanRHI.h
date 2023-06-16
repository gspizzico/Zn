#pragma once

#include <Core/CoreTypes.h>
#include <RHI/RHI.h>
#include <RHI/Vulkan/Vulkan.h>

namespace Zn
{
inline vk::SampleCountFlagBits TranslateSampleCount(SampleCount sampleCount_)
{
    return static_cast<vk::SampleCountFlagBits>(sampleCount_);
}

inline vk::ImageLayout TranslateImageLayout(ImageLayout layout_)
{
    switch (layout_)
    {
    case ImageLayout::ColorAttachment:
        return vk::ImageLayout::eColorAttachmentOptimal;
    case ImageLayout::DepthStencilAttachment:
        return vk::ImageLayout::eDepthStencilAttachmentOptimal;
    case ImageLayout::Present:
        return vk::ImageLayout::ePresentSrcKHR;
    }
    return vk::ImageLayout::eUndefined;
}

inline vk::PipelineStageFlags TranslatePipelineStage(PipelineStage stageMask_)
{
    return vk::PipelineStageFlags(static_cast<uint32>(stageMask_));
}

inline vk::AccessFlags TranslateAccessFlags(AccessFlag flags_)
{
    return vk::AccessFlags(static_cast<uint32>(flags_));
}

inline vk::ShaderStageFlagBits TranslateShaderStage(ShaderStage flag_)
{
    if (EnumHasAll(flag_, ShaderStage::Vertex))
        return vk::ShaderStageFlagBits::eVertex;

    if (EnumHasAll(flag_, ShaderStage::TessellationControl))
        return vk::ShaderStageFlagBits::eTessellationControl;

    if (EnumHasAll(flag_, ShaderStage::TessellationEvaluation))
        return vk::ShaderStageFlagBits::eTessellationEvaluation;

    if (EnumHasAll(flag_, ShaderStage::Geometry))
        return vk::ShaderStageFlagBits::eGeometry;

    if (EnumHasAll(flag_, ShaderStage::Fragment))
        return vk::ShaderStageFlagBits::eFragment;

    if (EnumHasAll(flag_, ShaderStage::Compute))
        return vk::ShaderStageFlagBits::eCompute;

    if (EnumHasAll(flag_, ShaderStage::Task))
        return vk::ShaderStageFlagBits::eTaskEXT;

    if (EnumHasAll(flag_, ShaderStage::Mesh))
        return vk::ShaderStageFlagBits::eMeshEXT;

    if (EnumHasAll(flag_, ShaderStage::Raygen))
        return vk::ShaderStageFlagBits::eRaygenKHR;

    if (EnumHasAll(flag_, ShaderStage::AnyHit))
        return vk::ShaderStageFlagBits::eAnyHitNV;

    if (EnumHasAll(flag_, ShaderStage::ClosestHit))
        return vk::ShaderStageFlagBits::eClosestHitNV;

    if (EnumHasAll(flag_, ShaderStage::Miss))
        return vk::ShaderStageFlagBits::eMissNV;

    if (EnumHasAll(flag_, ShaderStage::Intersection))
        return vk::ShaderStageFlagBits::eIntersectionNV;

    if (EnumHasAll(flag_, ShaderStage::Callable))
        return vk::ShaderStageFlagBits::eCallableNV;

    check(false);

    return vk::ShaderStageFlagBits::eVertex;
}

inline vk::ShaderStageFlags TranslateShaderStagesMask(ShaderStage flags_)
{
    if (flags_ == ShaderStage::All)
    {
        return vk::ShaderStageFlagBits::eAll;
    }

    static constexpr vk::ShaderStageFlags kNoFlag {0};

    vk::ShaderStageFlags outFlags = kNoFlag;

    outFlags |= EnumHasAll(flags_, ShaderStage::Vertex) ? vk::ShaderStageFlagBits::eVertex : kNoFlag;
    outFlags |= EnumHasAll(flags_, ShaderStage::TessellationControl) ? vk::ShaderStageFlagBits::eTessellationControl : kNoFlag;
    outFlags |= EnumHasAll(flags_, ShaderStage::TessellationEvaluation) ? vk::ShaderStageFlagBits::eTessellationEvaluation : kNoFlag;
    outFlags |= EnumHasAll(flags_, ShaderStage::Geometry) ? vk::ShaderStageFlagBits::eGeometry : kNoFlag;
    outFlags |= EnumHasAll(flags_, ShaderStage::Fragment) ? vk::ShaderStageFlagBits::eFragment : kNoFlag;
    outFlags |= EnumHasAll(flags_, ShaderStage::Compute) ? vk::ShaderStageFlagBits::eCompute : kNoFlag;
    outFlags |= EnumHasAll(flags_, ShaderStage::Task) ? vk::ShaderStageFlagBits::eTaskEXT : kNoFlag;
    outFlags |= EnumHasAll(flags_, ShaderStage::Mesh) ? vk::ShaderStageFlagBits::eMeshEXT : kNoFlag;
    outFlags |= EnumHasAll(flags_, ShaderStage::Raygen) ? vk::ShaderStageFlagBits::eRaygenKHR : kNoFlag;
    outFlags |= EnumHasAll(flags_, ShaderStage::AnyHit) ? vk::ShaderStageFlagBits::eAnyHitNV : kNoFlag;
    outFlags |= EnumHasAll(flags_, ShaderStage::ClosestHit) ? vk::ShaderStageFlagBits::eClosestHitNV : kNoFlag;
    outFlags |= EnumHasAll(flags_, ShaderStage::Miss) ? vk::ShaderStageFlagBits::eMissNV : kNoFlag;
    outFlags |= EnumHasAll(flags_, ShaderStage::Intersection) ? vk::ShaderStageFlagBits::eIntersectionNV : kNoFlag;
    outFlags |= EnumHasAll(flags_, ShaderStage::Callable) ? vk::ShaderStageFlagBits::eCallableNV : kNoFlag;

    return outFlags;
}

inline vk::ColorComponentFlags TranslateColorComponents(ColorComponent colors_)
{
    return (vk::ColorComponentFlags)(uint32(colors_));
}

inline vk::BlendFactor TranslateBlendFactor(BlendFactor blendFactor_)
{
    return (vk::BlendFactor)(uint32(blendFactor_));
}

inline vk::BlendOp TranslateBlendOp(BlendOp blendOp_)
{
    return (vk::BlendOp)(uint32(blendOp_));
}

inline vk::LogicOp TranslateLogicOp(LogicOp logicOp_)
{
    return (vk::LogicOp)(uint32(logicOp_));
}

inline vk::CompareOp TranslateCompareOp(CompareOp compareOp_)
{
    return (vk::CompareOp)(uint32(compareOp_));
}
} // namespace Zn
