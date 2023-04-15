#pragma once

#define ZN_VK_VALIDATION_LAYERS (ZN_DEBUG)
#define ZN_VK_VALIDATION_VERBOSE (0)

#define ZN_VK_CHECK(expression)\
if(expression != VK_SUCCESS)\
{\
_ASSERT(false);\
}

#define ZN_VK_CHECK_RETURN(expression)\
if(expression != VK_SUCCESS)\
{\
_ASSERT(false);\
return;\
}

#include <vulkan/vulkan.h>
#include <SDL_vulkan.h>