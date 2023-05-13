#pragma once

#include <Core/HAL/BasicTypes.h>

namespace Zn
{
struct ResourceHandle
{
    u64 handle = u64_max;

    constexpr ResourceHandle() = default;
    constexpr ResourceHandle(u64 h)
        : handle(h)
    {
    }

    operator bool() const
    {
        return handle != u64_max;
    }

    bool operator==(const Zn::ResourceHandle& other) const
    {
        return handle == other.handle;
    }

    bool operator!=(const Zn::ResourceHandle& other) const
    {
        return !(*this == other);
    }
};

struct RHIBuffer
{
    vk::Buffer      data;
    vma::Allocation allocation;

    RHIBuffer() = default;
    RHIBuffer(std::pair<vk::Buffer, vma::Allocation>&& kvp)
        : data(kvp.first)
        , allocation(kvp.second)
    {
    }

    operator bool() const
    {
        return data && allocation;
    }
};
} // namespace Zn

namespace std
{
template<>
struct hash<Zn::ResourceHandle>
{
    std::size_t operator()(const Zn::ResourceHandle& h) const
    {
        return static_cast<std::size_t>(h.handle);
    }
};
} // namespace std
