#pragma once

#include <Core/CoreMinimal.h>

namespace Zn
{
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
