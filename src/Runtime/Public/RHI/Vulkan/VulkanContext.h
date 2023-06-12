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
    vk::Fence         fences[NumCommandBuffers];
};

struct VulkanContext
{
    static VulkanContext& Get();

    vk::Instance                           instance;
    VulkanGPU                              gpu;
    vk::Device                             device;
    vk::SurfaceKHR                         surface;
    vma::Allocator                         allocator;
    VulkanCommandContext<kVkMaxImageCount> graphicsCmdContext;
    VulkanCommandContext<1>                uploadCmdContext;
};
} // namespace Zn
