#pragma once

#include <optional>
#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>
#include <glm/glm.hpp>

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

		struct VertexInputDescription
		{
			Vector<VkVertexInputBindingDescription> Bindings{};
			Vector<VkVertexInputAttributeDescription> Attributes{};
			VkPipelineVertexInputStateCreateFlags Flags{ 0 };
		};

		struct Vertex
		{
			glm::vec3 Position;
			glm::vec3 Normal;
			glm::vec3 Color;

			static VertexInputDescription GetVertexInputDescription();
		};

		struct MeshPushConstants
		{
			glm::vec4 Data;
			glm::mat4 RenderMatrix;
		};
	}
}
