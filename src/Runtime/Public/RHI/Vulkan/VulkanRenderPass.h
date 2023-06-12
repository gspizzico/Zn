#pragma once

#include <RHI/Vulkan/Vulkan.h>
#include <RHI/RHIRenderPass.h>

namespace Zn
{
struct VulkanSubpassDescription
{
    Vector<vk::AttachmentReference> inputAttachments {};
    Vector<vk::AttachmentReference> colorAttachments {};
    vk::AttachmentReference         depthStencilAttachment {};
    Vector<vk::AttachmentReference> resolveAttachments {};
    Span<const uint32>              preserveAttachments {};
};

struct VulkanRenderPassDescription
{
    Vector<vk::AttachmentDescription> attachments;
    Vector<VulkanSubpassDescription>  subpasses;
    Vector<vk::SubpassDescription>    vkSubpasses;
    Vector<vk::SubpassDependency>     dependencies;
};

struct VulkanRenderPass
{
    vk::RenderPass  renderPass;
    vk::Framebuffer frameBuffer[kVkMaxImageCount];
};

inline vk::SampleCountFlagBits TranslateSampleCount(SampleCount sampleCount_)
{
    return static_cast<vk::SampleCountFlagBits>(sampleCount_);
}
inline vk::AttachmentLoadOp TranslateLoadOp(LoadOp loadOp_)
{
    switch (loadOp_)
    {
    case LoadOp::Load:
        return vk::AttachmentLoadOp::eLoad;
    case LoadOp::Clear:
        return vk::AttachmentLoadOp::eClear;
    case LoadOp::DontCare:
        return vk::AttachmentLoadOp::eDontCare;
    }

    return vk::AttachmentLoadOp::eLoad;
}

inline vk::AttachmentStoreOp TranslateStoreOp(StoreOp storeOp_)
{
    switch (storeOp_)
    {
    case StoreOp::Store:
        return vk::AttachmentStoreOp::eStore;
    case StoreOp::DontCare:
        return vk::AttachmentStoreOp::eDontCare;
    case StoreOp::None:
        return vk::AttachmentStoreOp::eNone;
    }

    return vk::AttachmentStoreOp::eDontCare;
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

inline vk::PipelineBindPoint TranslatePipelineType(PipelineType type_)
{
    switch (type_)
    {
    case PipelineType::Graphics:
        return vk::PipelineBindPoint::eGraphics;
    case PipelineType::Compute:
        return vk::PipelineBindPoint::eCompute;
    case PipelineType::Raytracing:
        return vk::PipelineBindPoint::eRayTracingKHR;
    }

    checkMsg(false, "Pipeline Type is undefined");
    return vk::PipelineBindPoint::eGraphics;
}

vk::AttachmentDescription   TranslateAttachment(const RenderPassAttachment& description_);
vk::AttachmentReference     TranslateAttachmentReference(const RenderPassAttachmentReference& reference_);
vk::SubpassDependency       TranslateSubpassDependency(const SubpassDependency& dependency_);
VulkanRenderPassDescription TranslateRenderPass(const RHIRenderPassDescription& renderPass_);
} // namespace Zn
