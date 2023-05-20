#pragma once

#include <vulkan/vulkan.h>
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#define VULKAN_HPP_NO_SPACESHIP_OPERATOR
#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
// #include <vma/vk_mem_alloc.h>
#include <vk_mem_alloc.hpp>
#include <SDL_vulkan.h>

#define ZN_VK_VALIDATION_LAYERS  (ZN_DEBUG)
#define ZN_VK_VALIDATION_VERBOSE (0)

#define ZN_VK_CHECK(expression)                                                                                                            \
    if (vk::Result result = expression; result != vk::Result::eSuccess)                                                                    \
        _ASSERT(false);

#define ZN_VK_CHECK_RETURN(expression)                                                                                                     \
    if (vk::Result result = expression; result != vk::Result::eSuccess)                                                                    \
    {                                                                                                                                      \
        _ASSERT(false);                                                                                                                    \
        return;                                                                                                                            \
    }

DECLARE_LOG_CATEGORY(LogVulkan);
DECLARE_LOG_CATEGORY(LogVulkanValidation);
