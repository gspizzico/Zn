#pragma once

#include <optional>
#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

namespace Zn
{
	namespace Vk
	{
		struct QueueFamilyIndices
		{
			std::optional<uint32> Graphics;
			std::optional<uint32> Present;
		};

		struct SwapChainDetails
		{
			VkSurfaceCapabilitiesKHR Capabilities;
			Vector<VkSurfaceFormatKHR> Formats;
			Vector<VkPresentModeKHR> PresentModes;
		};

		struct AllocatedBuffer
		{
			VkBuffer Buffer;
			VmaAllocation Allocation;
		};
	}
}
