#pragma once

#include <Core/HAL/BasicTypes.h>
#include <Rendering/Vulkan/VulkanTypes.h>

namespace Zn
{
	struct RHITexture
	{
		i32 width;

		i32 height;

		vk::Image image;

		vk::ImageView imageView;

		vma::Allocation allocation;
	};
}