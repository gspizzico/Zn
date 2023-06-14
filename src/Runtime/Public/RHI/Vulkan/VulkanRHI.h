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

inline vk::ShaderStageFlags TranslateShaderStageFlags(ShaderStage flags_)
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
} // namespace Zn
