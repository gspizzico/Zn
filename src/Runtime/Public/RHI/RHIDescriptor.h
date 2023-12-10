#pragma once

#include <Core/CoreTypes.h>
#include <RHI/RHI.h>

namespace Zn::RHI
{
enum class DescriptorType : uint32
{
    Sampler,
    CombinedImageSampler,
    SampledImage,
    StorageImage,
    UniformTexelBuffer,
    StorageTexelBuffer,
    UniformBuffer,
    StorageBuffer,
    UniformBufferDynamic,
    StorageBufferDynamic,
    InputAttachment,
    InlineUniformBlock,
    // InlineUniformBlockEXT,
    AccelerationStructure,
    // AccelerationStructureNV,
    // MutableVALVE,
    // SampleWeightImageQCOM,
    // BlockMatchImageQCOM,
    // MutableEXT,
};

enum class DescriptorPoolCreateFlag : uint32
{
    None              = 0,
    FreeDescriptorSet = 0b1,
    UpdateAfterBind   = 0b10,
    HostOnly          = 0b100
};

ENABLE_BITMASK_OPERATORS(DescriptorPoolCreateFlag);

// TODO: Might want to use different name for *Description structure
struct DescriptorPoolDescription
{
    Span<const std::pair<RHI::DescriptorType, uint32>> desc;
    DescriptorPoolCreateFlag                           flags;
    uint32                                             maxSets;
};

struct DescriptorSetLayoutBinding
{
    uint32              binding;
    RHI::DescriptorType descriptorType;
    uint32              descriptorCount;
    ShaderStage         shaderStages;
};

struct DescriptorSetLayoutDescription
{
    Span<const RHI::DescriptorSetLayoutBinding> bindings;
};

struct DescriptorSetAllocationDescription
{
    DescriptorPoolHandle                  descriptorPool;
    Span<const DescriptorSetLayoutHandle> descriptorSetLayouts;
};

struct DescriptorBufferInfo
{
    union
    {
        UBOHandle ubo;
    };

    uint32 offset;
    uint32 range;
};

struct DescriptorSetUpdateDescription
{
    DescriptorSetHandle              descriptorSet;
    uint32                           binding;
    uint32                           arrayElement;
    RHI::DescriptorType              descriptorType;
    Span<const DescriptorBufferInfo> descriptorBufferInfo;
};
} // namespace Zn::RHI
