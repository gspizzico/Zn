#pragma once

#include <RHI/RHIResource.h>
#include <RHI/Vulkan/Vulkan.h>

namespace Zn
{
vma::MemoryUsage TranslateRHIResourceUsage(RHIResourceUsage usage_)
{
    switch (usage_)
    {
    case RHIResourceUsage::Unknown:
        return vma::MemoryUsage::eUnknown;
    case RHIResourceUsage::Gpu:
        return vma::MemoryUsage::eGpuOnly;
    case RHIResourceUsage::Cpu:
        return vma::MemoryUsage::eCpuOnly;
    case RHIResourceUsage::CpuToGpu:
        return vma::MemoryUsage::eCpuToGpu;
    case RHIResourceUsage::GpuToCpu:
        return vma::MemoryUsage::eGpuToCpu;
    case RHIResourceUsage::Auto:
        return vma::MemoryUsage::eAuto;
    case RHIResourceUsage::AutoPreferGpu:
        return vma::MemoryUsage::eAutoPreferDevice;
    case RHIResourceUsage::AutoPreferCpu:
        return vma::MemoryUsage::eAutoPreferHost;
    }

    return vma::MemoryUsage::eAuto;
}
} // namespace Zn
