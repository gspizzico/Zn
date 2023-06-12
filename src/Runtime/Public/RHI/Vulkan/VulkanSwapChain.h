#pragma once

#include <Core/CoreTypes.h>
#include <RHI/Vulkan/Vulkan.h>

namespace Zn
{
struct VulkanSwapChain
{
    vk::SwapchainKHR     swapChain     = nullptr;
    vk::PresentModeKHR   presentMode   = vk::PresentModeKHR::eFifo;
    vk::SurfaceFormatKHR surfaceFormat = {vk::Format::eUndefined, vk::ColorSpaceKHR::eSrgbNonlinear};
    vk::Extent2D         extent {0, 0};
    uint32               imageCount = 0;
    vk::Image            images[kVkMaxImageCount];
    vk::ImageView        imageViews[kVkMaxImageCount];

    void Create();
    void Destroy();
};
} // namespace Zn
