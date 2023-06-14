#pragma once

#include <Core/CoreTypes.h>
#include <RHI/RHI.h>
#include <RHI/RHITexture.h>

namespace Zn
{
enum class AttachmentType : uint8
{
    Undefined = 0,
    Color     = 0b1,
    Depth     = 0b10,
    Stencil   = 0b100
};

enum class LoadOp : uint8
{
    Load     = 0,
    Clear    = 1,
    DontCare = 2,
};

enum class StoreOp : uint8
{
    Store    = 0,
    DontCare = 1,
    None     = 2
};

struct RenderPassAttachment
{
    AttachmentType type          = AttachmentType::Undefined;
    RHIFormat      format        = RHIFormat::Undefined;
    SampleCount    samples       = SampleCount::s1;
    LoadOp         loadOp        = LoadOp::Load;
    StoreOp        storeOp       = StoreOp::Store;
    ImageLayout    initialLayout = ImageLayout::Undefined;
    ImageLayout    finalLayout   = ImageLayout::Undefined;
};

struct RenderPassAttachmentReference
{
    uint32      attachment = 0;
    ImageLayout layout     = ImageLayout::Undefined;
};

struct SubpassDependency
{
    static constexpr uint32 kExternalSubpass = u32_max;

    uint32        srcSubpass    = 0;
    uint32        dstSubpass    = 0;
    PipelineStage srcStageMask  = PipelineStage::None;
    PipelineStage dstStageMask  = PipelineStage::None;
    AccessFlag    srcAccessMask = AccessFlag::None;
    AccessFlag    dstAccessMask = AccessFlag::None;
};

struct SubpassDescription
{
    PipelineType                              pipeline = PipelineType::Undefined;
    Span<const RenderPassAttachmentReference> inputAttachments {};
    Span<const RenderPassAttachmentReference> colorAttachments {};
    RenderPassAttachmentReference             depthStencilAttachment {};
    Span<const RenderPassAttachmentReference> resolveAttachments {}; // TODO: check same size as colorAttachments
    Span<const uint32>                        preserveAttachments {};
};

struct RHIRenderPassDescription
{
    Span<const RenderPassAttachment> attachments;
    Span<const SubpassDescription>   subpasses;
    Span<const SubpassDependency>    dependencies;
};

} // namespace Zn
