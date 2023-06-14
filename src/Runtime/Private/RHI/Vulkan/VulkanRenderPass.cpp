#include <RHI/Vulkan/VulkanRenderPass.h>
#include <RHI/Vulkan/VulkanRHI.h>
#include <RHI/Vulkan/VulkanTexture.h>

vk::AttachmentDescription Zn::TranslateAttachment(const RenderPassAttachment& description_)
{
    return vk::AttachmentDescription {
        .format        = TranslateRHIFormat(description_.format),
        .samples       = TranslateSampleCount(description_.samples),
        .loadOp        = TranslateLoadOp(description_.loadOp),
        .storeOp       = TranslateStoreOp(description_.storeOp),
        .stencilLoadOp = description_.type == AttachmentType::Stencil || description_.type == AttachmentType::Depth
                             ? TranslateLoadOp(description_.loadOp)
                             : vk::AttachmentLoadOp::eDontCare,
        .stencilStoreOp =
            description_.type == AttachmentType::Stencil ? TranslateStoreOp(description_.storeOp) : vk::AttachmentStoreOp::eDontCare,
        .initialLayout = TranslateImageLayout(description_.initialLayout),
        .finalLayout   = TranslateImageLayout(description_.finalLayout),
    };
}

vk::AttachmentReference Zn::TranslateAttachmentReference(const RenderPassAttachmentReference& reference_)
{
    return vk::AttachmentReference {
        .attachment = reference_.attachment,
        .layout     = TranslateImageLayout(reference_.layout),
    };
}

vk::SubpassDependency Zn::TranslateSubpassDependency(const SubpassDependency& dependency_)
{
    return vk::SubpassDependency {
        .srcSubpass    = dependency_.srcSubpass == SubpassDependency::kExternalSubpass ? VK_SUBPASS_EXTERNAL : dependency_.srcSubpass,
        .dstSubpass    = dependency_.dstSubpass == SubpassDependency::kExternalSubpass ? VK_SUBPASS_EXTERNAL : dependency_.dstSubpass,
        .srcStageMask  = TranslatePipelineStage(dependency_.srcStageMask),
        .dstStageMask  = TranslatePipelineStage(dependency_.dstStageMask),
        .srcAccessMask = TranslateAccessFlags(dependency_.srcAccessMask),
        .dstAccessMask = TranslateAccessFlags(dependency_.dstAccessMask),
    };
}

Zn::VulkanRenderPassDescription Zn::TranslateRenderPass(const RHIRenderPassDescription& renderPass_)
{
    VulkanRenderPassDescription vkRenderPass;
    for (const RenderPassAttachment& attachment : renderPass_.attachments)
    {
        vkRenderPass.attachments.emplace_back(TranslateAttachment(attachment));
    }

    for (const SubpassDescription& subpass : renderPass_.subpasses)
    {
        VulkanSubpassDescription& vkSubpass = vkRenderPass.subpasses.emplace_back(VulkanSubpassDescription {});

        for (const RenderPassAttachmentReference& ref : subpass.inputAttachments)
        {
            vkSubpass.inputAttachments.push_back(TranslateAttachmentReference(ref));
        }

        for (const RenderPassAttachmentReference& ref : subpass.colorAttachments)
        {
            vkSubpass.colorAttachments.push_back(TranslateAttachmentReference(ref));
        }

        vkSubpass.depthStencilAttachment = TranslateAttachmentReference(subpass.depthStencilAttachment);

        for (const RenderPassAttachmentReference& ref : subpass.resolveAttachments)
        {
            vkSubpass.resolveAttachments.push_back(TranslateAttachmentReference(ref));
        }

        vkSubpass.preserveAttachments = subpass.preserveAttachments;

        vk::SubpassDescription desc {
            .pipelineBindPoint = TranslatePipelineType(subpass.pipeline),
        };

        desc.setInputAttachments(vkSubpass.inputAttachments);
        desc.setColorAttachments(vkSubpass.colorAttachments);
        desc.setPDepthStencilAttachment(&vkSubpass.depthStencilAttachment);
        desc.setPreserveAttachmentCount(static_cast<uint32>(vkSubpass.preserveAttachments.Size()));
        desc.setPPreserveAttachments(vkSubpass.preserveAttachments.Data());

        vkRenderPass.vkSubpasses.emplace_back(std::move(desc));
    }

    for (const SubpassDependency& dep : renderPass_.dependencies)
    {
        vkRenderPass.dependencies.push_back(TranslateSubpassDependency(dep));
    }

    return vkRenderPass;
}
