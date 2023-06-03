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
    static constexpr uint64 kInvalidHandle = u64_max;

    uint64 handle = kInvalidHandle;

    operator bool() const
    {
        return handle != u64_max;
    }
};

template<RHIResourceType type>
struct TResourceHandle : ResourceHandle
{
    TResourceHandle(ResourceHandle&& other_)
    {
        handle        = other_.handle;
        other_.handle = kInvalidHandle;
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

namespace MAP_HASH_NAMESPACE
{
template<>
struct hash<Zn::ResourceHandle>
{
    using is_avalanching = void;
    auto operator()(Zn::ResourceHandle const& handle_) const noexcept -> uint64
    {
        return handle_.handle;
    }
};
} // namespace MAP_HASH_NAMESPACE
