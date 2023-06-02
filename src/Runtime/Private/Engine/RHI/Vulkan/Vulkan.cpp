#include <RHI/Vulkan/Vulkan.h>
#include <Core/Log/LogMacros.h>

#define VMA_IMPLEMENTATION
// #define VMA_DEBUG_INITIALIZE_ALLOCATIONS 1
#include <vk_mem_alloc.h>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

DEFINE_LOG_CATEGORY(LogVulkan, Zn::ELogVerbosity::Log);
DEFINE_LOG_CATEGORY(LogVulkanValidation, Zn::ELogVerbosity::Verbose);
