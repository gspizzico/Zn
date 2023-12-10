#pragma once

#include <Core/CoreTypes.h>

namespace Zn
{
enum class RHIResourceType
{
    Texture,
    VertexBuffer,
    IndexBuffer,
    UniformBuffer,
    StagingBuffer,
    // TODO: These are not really resources.
    RenderPass,
    DescriptorPool,
    DescriptorSetLayout,
    DescriptorSet,
    ShaderModule,
    Pipeline,
};

enum class RHIResourceUsage
{
    Unknown,
    Gpu,
    Cpu,
    CpuToGpu,
    GpuToCpu,
    // eCpuCopy            = VMA_MEMORY_USAGE_CPU_COPY,
    // eGpuLazilyAllocated = VMA_MEMORY_USAGE_GPU_LAZILY_ALLOCATED,
    Auto,
    AutoPreferGpu,
    AutoPreferCpu
};

struct ResourceHandle
{
    static constexpr uint16 kInvalidGen = u16_max;

    uint16 gen   = u16_max;
    uint16 index = 0;

    operator bool() const
    {
        return gen != u64_max;
    }
};

template<RHIResourceType type>
struct TResourceHandle : ResourceHandle
{
    TResourceHandle() = default;
    TResourceHandle(ResourceHandle&& other_)
    {
        gen   = other_.gen;
        index = other_.index;

        other_.gen   = kInvalidGen;
        other_.index = 0;
    }

    TResourceHandle(const TResourceHandle& other_)
    {
        gen   = other_.gen;
        index = other_.index;
    }
};

using TextureHandle             = TResourceHandle<RHIResourceType::Texture>;
using UBOHandle                 = TResourceHandle<RHIResourceType::UniformBuffer>;
using VertexBufferHandle        = TResourceHandle<RHIResourceType::VertexBuffer>;
using IndexBufferHandle         = TResourceHandle<RHIResourceType::IndexBuffer>;
// TODO: These are not really resources.
using RenderPassHandle          = TResourceHandle<RHIResourceType::RenderPass>;
using DescriptorPoolHandle      = TResourceHandle<RHIResourceType::DescriptorPool>;
using DescriptorSetLayoutHandle = TResourceHandle<RHIResourceType::DescriptorSetLayout>;
using DescriptorSetHandle       = TResourceHandle<RHIResourceType::DescriptorSet>;
using ShaderModuleHandle        = TResourceHandle<RHIResourceType::ShaderModule>;
using PipelineHandle            = TResourceHandle<RHIResourceType::Pipeline>;

template<typename TRHI, typename TNativeRHI>
struct TResource
{
    TRHI       resource;
    TNativeRHI payload;
};
} // namespace Zn
