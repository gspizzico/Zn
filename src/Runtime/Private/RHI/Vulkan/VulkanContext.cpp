#include <RHI/Vulkan/VulkanContext.h>

using namespace Zn;

namespace
{
VulkanContext GVulkanContext {};
}

VulkanContext& VulkanContext::Get()
{
    return GVulkanContext;
}
