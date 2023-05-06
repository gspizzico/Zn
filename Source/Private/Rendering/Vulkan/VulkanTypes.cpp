#include <Znpch.h>
#include <Rendering/Vulkan/VulkanTypes.h>

// TODO: MOVE AWAY FROM HERE
#ifndef STB_IMAGE_IMPLEMENTATION
	#define STB_IMAGE_IMPLEMENTATION
#endif
#include <stb_image.h>

using namespace Zn::Vk;

VertexInputDescription Vertex::GetVertexInputDescription()
{
	VertexInputDescription description;

	//we will have just 1 vertex buffer binding, with a per-vertex rate
	description.Bindings =
	{
		{
			.binding = 0,
			.stride = sizeof(Vertex),
			.inputRate = vk::VertexInputRate::eVertex,
		}
	};

	description.Attributes =
	{
		{
			.location = 0,
			.binding = 0,
			.format = vk::Format::eR32G32B32A32Sfloat,
			.offset = offsetof(Vertex, Position)
		},
		{
			.location = 1,
			.binding = 0,
			.format = vk::Format::eR32G32B32A32Sfloat,
			.offset = offsetof(Vertex, Normal)
		},
		{
			.location = 2,
			.binding = 0,
			.format = vk::Format::eR32G32B32A32Sfloat,
			.offset = offsetof(Vertex, Color)
		},
		{
			.location = 3,
			.binding = 0,
			.format = vk::Format::eR32G32Sfloat,
			.offset = offsetof(Vertex, UV)
		},
	};

	return description;
}

vk::ImageCreateInfo AllocatedImage::GetImageCreateInfo(vk::Format inFormat, vk::ImageUsageFlags inUsageFlags, vk::Extent3D inExtent)
{
	//	.format = texture data type, like holding a single float(for depth), or holding color.
	//	.extent = size of the image, in pixels.
	//	.mipLevels = num of mipmap levels the image has. TODO: Because we aren’t using them here, we leave the levels to 1.
	//	.arrayLayers = used for layered textures.
	//					You can create textures that are many - in - one, using layers. 
	//					An example of layered textures is cubemaps, where you have 6 layers, one layer for each face of the cubemap.
	//					We default it to 1 layer because we aren’t doing cubemaps.
	//	.samples = controls the MSAA behavior of the texture. This only makes sense for render targets, such as depth images and images you are rendering to. 
	//					TODO: We won’t be doing MSAA in this tutorial, so samples will be kept at 1 sample.
	//  .tiling = if you use VK_IMAGE_TILING_OPTIMAL, it won’t be possible to read the data from CPU or to write it without changing its tiling first 
	//					(it’s possible to change the tiling of a texture at any point, but this can be a costly operation). 
	//					The other tiling we can care about is VK_IMAGE_TILING_LINEAR, which will store the image as a 2d array of pixels. 
	//	.usage = controls how the GPU handles the image memory.
	
	return {
		.imageType = vk::ImageType::e2D,
		.format = inFormat,
		.extent = inExtent,
		.mipLevels = 1,
		.arrayLayers = 1,
		.samples = vk::SampleCountFlagBits::e1,
		.tiling = vk::ImageTiling::eOptimal,
		.usage = inUsageFlags
	}; 
}

vk::ImageViewCreateInfo AllocatedImage::GetImageViewCreateInfo(vk::Format InFormat, vk::Image InImage, vk::ImageAspectFlagBits InAspectFlags)
{

	return {
		.image = InImage,
		//	While imageType held the dimensionality of the texture, viewType has a lot more options, like VK_IMAGE_VIEW_TYPE_CUBE for cubemaps. 
		.viewType = vk::ImageViewType::e2D,
		//	TODO: In here, we will have it matched to GetImageCreateInfo, and hardcode it to 2D images as it’s the most common case.
		.format = InFormat,
		.subresourceRange = 
		{
			.aspectMask = InAspectFlags,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1,
		}
	};
}

bool Zn::Vk::RawTexture::LoadFromFile(String path, RawTexture& outTexture)
{
	outTexture.data = stbi_load(path.c_str(), &outTexture.width, &outTexture.height, &outTexture.channels, STBI_rgb_alpha);

	const bool success = outTexture.data != nullptr;

	if (success)
	{
		outTexture.size = outTexture.width * outTexture.height * 4;
	}

	return success;
}

void Zn::Vk::RawTexture::Unload(RawTexture& inTexture)
{
	if (inTexture.data != nullptr)
	{
		stbi_image_free(inTexture.data);
	}

	memset(&inTexture, 0, sizeof(inTexture));
}
