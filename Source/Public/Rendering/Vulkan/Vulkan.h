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

namespace Zn
{
	//struct VulkanContext
	//{
	//	static constexpr u32 k_max_concurrent_frames = 2;

	//	VkInstance instance{ VK_NULL_HANDLE };

	//	VmaAllocator allocator{ VK_NULL_HANDLE };

	//	VkDevice logical_device{ VK_NULL_HANDLE };

	//	VkPhysicalDevice gpu{ VK_NULL_HANDLE };

	//	VkSurfaceKHR surface{ VK_NULL_HANDLE };

	//	VkQueue queue_graphics{ VK_NULL_HANDLE };

	//	VkQueue queue_present{ VK_NULL_HANDLE };

	//	VkSwapchainKHR swapchain{ VK_NULL_HANDLE };

	//	VkSurfaceFormatKHR swapchain_format{};

	//	VkExtent2D swapchain_extent{};

	//	Vector<VkImage> swapchain_images{};

	//	Vector<VkImageView> swapchain_image_views{};

	//	VkCommandPool command_pool{ VK_NULL_HANDLE };

	//	VkCommandBuffer command_buffers[k_max_concurrent_frames]{ VK_NULL_HANDLE };

	//	VkSemaphore semaphores_present[k_max_concurrent_frames]{ VK_NULL_HANDLE };

	//	VkSemaphore semaphores_render[k_max_concurrent_frames]{ VK_NULL_HANDLE };

	//	VkFence fences_render[k_max_concurrent_frames];
	//};
}