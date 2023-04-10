#pragma once

#include <vulkan/vulkan.h>
#include <Rendering/Vulkan/VulkanTypes.h>

namespace Zn
{
	namespace Vk
	{
		namespace Obj
		{
			bool LoadMesh(String InFilename, Mesh& OutMesh);
		}
	}

}
