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

inline vk::Format TranslateRHIFormat(RHIFormat format_)
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

inline RHIFormat TranslateVkFormat(vk::Format format_)
{
    switch (format_)
    {
    case vk::Format::eB8G8R8A8Srgb:
        return RHIFormat::BGRA8_SRGB;
    case vk::Format::eR8G8B8A8Unorm:
        return RHIFormat::RGBA8_UNorm;
    case vk::Format::eD32Sfloat:
        return RHIFormat::D32_Float;
    case vk::Format::eUndefined:
        return RHIFormat::Undefined;
    }

    check(false);
    return RHIFormat::Undefined;
}

inline vk::ImageUsageFlags TranslateRHITextureFlags(RHITextureFlags flags_)
{
    static constexpr vk::ImageUsageFlags kNoFlag {0};

    vk::ImageUsageFlags outFlags = kNoFlag;

    outFlags |= EnumHasAll(flags_, RHITextureFlags::TransferSrc) ? vk::ImageUsageFlagBits::eTransferSrc : kNoFlag;
    outFlags |= EnumHasAll(flags_, RHITextureFlags::TransferDst) ? vk::ImageUsageFlagBits::eTransferDst : kNoFlag;
    outFlags |= EnumHasAll(flags_, RHITextureFlags::Sampled) ? vk::ImageUsageFlagBits::eSampled : kNoFlag;
    outFlags |= EnumHasAll(flags_, RHITextureFlags::Storage) ? vk::ImageUsageFlagBits::eStorage : kNoFlag;
    outFlags |= EnumHasAll(flags_, RHITextureFlags::Color) ? vk::ImageUsageFlagBits::eColorAttachment : kNoFlag;
    outFlags |= EnumHasAll(flags_, RHITextureFlags::DepthStencil) ? vk::ImageUsageFlagBits::eDepthStencilAttachment : kNoFlag;
    outFlags |= EnumHasAll(flags_, RHITextureFlags::Transient) ? vk::ImageUsageFlagBits::eTransientAttachment : kNoFlag;
    outFlags |= EnumHasAll(flags_, RHITextureFlags::Input) ? vk::ImageUsageFlagBits::eInputAttachment : kNoFlag;

    return outFlags;
}
} // namespace Zn
