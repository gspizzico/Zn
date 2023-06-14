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
} // namespace Zn
