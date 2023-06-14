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
