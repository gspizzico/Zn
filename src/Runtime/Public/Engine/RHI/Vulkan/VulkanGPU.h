#pragma once

#include <Core/CoreTypes.h>
#include <Engine/RHI/Vulkan/Vulkan.h>

namespace Zn
{

struct VulkanGPU
{
    VulkanGPU(vk::Instance instance_, vk::SurfaceKHR surface_);
    ~VulkanGPU();

    vk::Device CreateDevice();

    vk::PhysicalDevice         gpu;
    vk::PhysicalDeviceFeatures features;

    uint32 graphicsQueue = u32_max;
    uint32 presentQueue  = u32_max;
};
} // namespace Zn
