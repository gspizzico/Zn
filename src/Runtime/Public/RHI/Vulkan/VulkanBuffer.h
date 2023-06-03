#pragma once

#include <RHI/Vulkan/Vulkan.h>
#include <RHI/RHIBuffer.h>

namespace Zn
{
struct VulkanBuffer
{
    vk::Buffer      data;
    vma::Allocation memory;
};

vk::BufferUsageFlags TranslateRHIBufferUsage(RHIBufferUsage flags_)
{
    static constexpr vk::BufferUsageFlags kNoFlag {0};

    vk::BufferUsageFlags outFlags {0};

    outFlags |= (uint32) flags_ & (uint32) RHIBufferUsage::TransferSrc ? vk::BufferUsageFlagBits::eTransferSrc : kNoFlag;
    outFlags |= (uint32) flags_ & (uint32) RHIBufferUsage::TransferDst ? vk::BufferUsageFlagBits::eTransferDst : kNoFlag;
    outFlags |= (uint32) flags_ & (uint32) RHIBufferUsage::Uniform ? vk::BufferUsageFlagBits::eUniformBuffer : kNoFlag;
    outFlags |= (uint32) flags_ & (uint32) RHIBufferUsage::Storage ? vk::BufferUsageFlagBits::eStorageBuffer : kNoFlag;
    outFlags |= (uint32) flags_ & (uint32) RHIBufferUsage::Indirect ? vk::BufferUsageFlagBits::eIndirectBuffer : kNoFlag;
    outFlags |= (uint32) flags_ & (uint32) RHIBufferUsage::Vertex ? vk::BufferUsageFlagBits::eVertexBuffer : kNoFlag;
    outFlags |= (uint32) flags_ & (uint32) RHIBufferUsage::Index ? vk::BufferUsageFlagBits::eIndexBuffer : kNoFlag;

    return outFlags;
}
} // namespace Zn
