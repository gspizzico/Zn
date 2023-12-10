#pragma once

#include <Core/CoreTypes.h>
#include <RHI/Vulkan/Vulkan.h>
#include <RHI/RHIDescriptor.h>

namespace Zn::RHI
{
inline vk::DescriptorType TranslateDescriptorType(RHI::DescriptorType descriptorType_)
{
    switch (descriptorType_)
    {
    case RHI::DescriptorType::Sampler:
        return vk::DescriptorType::eSampler;
    case RHI::DescriptorType::CombinedImageSampler:
        return vk::DescriptorType::eCombinedImageSampler;
    case RHI::DescriptorType::SampledImage:
        return vk::DescriptorType::eSampledImage;
    case RHI::DescriptorType::StorageImage:
        return vk::DescriptorType::eStorageImage;
    case RHI::DescriptorType::UniformTexelBuffer:
        return vk::DescriptorType::eUniformTexelBuffer;
    case RHI::DescriptorType::StorageTexelBuffer:
        return vk::DescriptorType::eStorageTexelBuffer;
    case RHI::DescriptorType::UniformBuffer:
        return vk::DescriptorType::eUniformBuffer;
    case RHI::DescriptorType::StorageBuffer:
        return vk::DescriptorType::eStorageBuffer;
    case RHI::DescriptorType::UniformBufferDynamic:
        return vk::DescriptorType::eUniformBufferDynamic;
    case RHI::DescriptorType::StorageBufferDynamic:
        return vk::DescriptorType::eStorageBufferDynamic;
    case RHI::DescriptorType::InputAttachment:
        return vk::DescriptorType::eInputAttachment;
    case RHI::DescriptorType::InlineUniformBlock:
        return vk::DescriptorType::eInlineUniformBlock;
    case RHI::DescriptorType::AccelerationStructure:
        return vk::DescriptorType::eAccelerationStructureKHR;
    }

    checkMsg(false, "Invalid descriptor type");

    return vk::DescriptorType::eSampler;
}

inline vk::DescriptorPoolCreateFlags TranslateDescriptorPoolCreateFlags(RHI::DescriptorPoolCreateFlag flags_)
{
    vk::DescriptorPoolCreateFlags outFlags {0};

    if (EnumHasAll(flags_, RHI::DescriptorPoolCreateFlag::FreeDescriptorSet))
        outFlags |= vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;

    if (EnumHasAll(flags_, RHI::DescriptorPoolCreateFlag::UpdateAfterBind))
        outFlags |= vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind;

    if (EnumHasAll(flags_, RHI::DescriptorPoolCreateFlag::HostOnly))
        outFlags |= vk::DescriptorPoolCreateFlagBits::eHostOnlyEXT;

    return outFlags;
}
} // namespace Zn::RHI
