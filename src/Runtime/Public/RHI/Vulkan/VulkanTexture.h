#pragma once

#include <RHI/Vulkan/Vulkan.h>
#include <RHI/RHITexture.h>

namespace Zn
{
struct VulkanTexture
{
    vk::Image       image;
    vk::ImageView   imageView;
    vma::Allocation memory;
};

vk::Format TranslateRHIFormat(RHIFormat format_)
{
    switch (format_)
    {
    case RHIFormat::BGRA8_SRGB:
        return vk::Format::eB8G8R8A8Srgb;
    case RHIFormat::RGBA8_UNorm:
        return vk::Format::eR8G8B8A8Unorm;
    case RHIFormat::D32_Float:
        return vk::Format::eD32Sfloat;
    case RHIFormat::Undefined:
        return vk::Format::eUndefined;
    }

    check(false);

    return vk::Format::eUndefined;
}

vk::ImageUsageFlags TranslateRHITextureFlags(RHITextureFlags flags_)
{
    static constexpr vk::ImageUsageFlags kNoFlag {0};

    vk::ImageUsageFlags outFlags = kNoFlag;

    outFlags |= ((uint32) flags_ & (uint32) RHITextureFlags::TransferSrc) ? vk::ImageUsageFlagBits::eTransferSrc : kNoFlag;
    outFlags |= ((uint32) flags_ & (uint32) RHITextureFlags::TransferDst) ? vk::ImageUsageFlagBits::eTransferDst : kNoFlag;
    outFlags |= ((uint32) flags_ & (uint32) RHITextureFlags::Sampled) ? vk::ImageUsageFlagBits::eSampled : kNoFlag;
    outFlags |= ((uint32) flags_ & (uint32) RHITextureFlags::Storage) ? vk::ImageUsageFlagBits::eStorage : kNoFlag;
    outFlags |= ((uint32) flags_ & (uint32) RHITextureFlags::Color) ? vk::ImageUsageFlagBits::eColorAttachment : kNoFlag;
    outFlags |= ((uint32) flags_ & (uint32) RHITextureFlags::DepthStencil) ? vk::ImageUsageFlagBits::eDepthStencilAttachment : kNoFlag;
    outFlags |= ((uint32) flags_ & (uint32) RHITextureFlags::Transient) ? vk::ImageUsageFlagBits::eTransientAttachment : kNoFlag;
    outFlags |= ((uint32) flags_ & (uint32) RHITextureFlags::Input) ? vk::ImageUsageFlagBits::eInputAttachment : kNoFlag;

    return outFlags;
}
} // namespace Zn
