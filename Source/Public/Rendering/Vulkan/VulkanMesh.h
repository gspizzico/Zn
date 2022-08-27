#pragma once

#include <vulkan/vulkan.h>
#include <Rendering/Vulkan/VulkanTypes.h>

namespace Zn
{
	namespace Vk
	{
		struct Mesh
		{
			Vector<Vertex> Vertices;

			AllocatedBuffer Buffer;
		};
	}

}
