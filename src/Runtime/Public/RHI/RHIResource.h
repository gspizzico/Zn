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
    StagingBuffer
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
};

using TextureHandle      = TResourceHandle<RHIResourceType::Texture>;
using UBOHandle          = TResourceHandle<RHIResourceType::UniformBuffer>;
using VertexBufferHandle = TResourceHandle<RHIResourceType::VertexBuffer>;
using IndexBufferHandle  = TResourceHandle<RHIResourceType::IndexBuffer>;

template<typename TRHI, typename TNativeRHI>
struct TResource
{
    TRHI       resource;
    TNativeRHI payload;
};
} // namespace Zn
