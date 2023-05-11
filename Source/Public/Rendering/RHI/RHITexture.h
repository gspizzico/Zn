#pragma once

#include <Core/HAL/BasicTypes.h>
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

    vma::Allocation allocation;
};
} // namespace Zn