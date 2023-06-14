#pragma once

#include <RHI/Vulkan/Vulkan.h>
#include <RHI/Vulkan/VulkanGPU.h>

namespace Zn
{
template<sizet NumCommandBuffers>
struct VulkanCommandContext
{
    vk::CommandPool   commandPool;
    vk::CommandBuffer commandBuffers[NumCommandBuffers];
};

struct VulkanContext
{
    static VulkanContext& Get();

    vk::Instance                           instance;
    VulkanGPU                              gpu;
    vk::Device                             device;
    vk::Queue                              graphicsQueue;
    vk::Queue                              presentQueue;
    vk::Queue                              computeQueue;
    vk::Queue                              transferQueue;
    vk::SurfaceKHR                         surface;
    vma::Allocator                         allocator;
    VulkanCommandContext<kVkMaxImageCount> graphicsCmdContext;
    VulkanCommandContext<1>                uploadCmdContext;
    vk::Fence                              uploadFence;
};
} // namespace Zn
