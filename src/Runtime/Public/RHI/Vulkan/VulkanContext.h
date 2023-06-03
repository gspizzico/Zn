#pragma once

#include <RHI/Vulkan/Vulkan.h>
#include <RHI/Vulkan/VulkanGPU.h>

namespace Zn
{
struct VulkanContext
{
    static VulkanContext& Get();

    vk::Instance   instance;
    VulkanGPU      gpu;
    vk::Device     device;
    vk::SurfaceKHR surface;
    vma::Allocator allocator;
};
} // namespace Zn
