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

// TODO: Might want to use different name for *Description structure
struct DescriptorPoolDescription
{
    Span<std::pair<RHI::DescriptorType, uint32>> desc;
    uint32                                       maxSets;
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
    Span<RHI::DescriptorSetLayoutBinding> bindings;
};
} // namespace Zn::RHI
