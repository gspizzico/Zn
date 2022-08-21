#pragma once

#include <vulkan/vulkan.h>
#include <Rendering/Vulkan/VulkanTypes.h>

namespace Zn
{
	namespace Vk
	{
		struct VertexInputDescription
		{
			Vector<VkVertexInputBindingDescription> Bindings{};
			Vector<VkVertexInputAttributeDescription> Attributes{};
			VkPipelineVertexInputStateCreateFlags Flags{0};
		};

		struct Vertex
		{
			float Position[3];
			float Normal[3];
			float Color[3];

			static VertexInputDescription GetVertexInputDescription();
		};

		struct Mesh
		{
			Vector<Vertex> Vertices;

			AllocatedBuffer Buffer;
		};
	}

}
