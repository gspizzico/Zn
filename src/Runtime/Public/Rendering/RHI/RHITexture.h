#pragma once

#include <Core/CoreTypes.h>
#include <Rendering/RHI/Vulkan/Vulkan.h>

namespace Zn
{
struct RHITexture
{
    i32 width;

    i32 height;

    vk::Format format;

    vk::Image image;

    vk::ImageView imageView;

    vk::Sampler sampler;

    vma::Allocation allocation;
};
} // namespace Zn
