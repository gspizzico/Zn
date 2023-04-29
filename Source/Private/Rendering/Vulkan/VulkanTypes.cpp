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
	VkVertexInputBindingDescription mainBinding = {};
	mainBinding.binding = 0;
	mainBinding.stride = sizeof(Vertex);
	mainBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	description.Bindings.push_back(mainBinding);

	//Position will be stored at Location 0
	VkVertexInputAttributeDescription positionAttribute = {};
	positionAttribute.binding = 0;
	positionAttribute.location = 0;
	positionAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
	positionAttribute.offset = offsetof(Vertex, Position);

	//Normal will be stored at Location 1
	VkVertexInputAttributeDescription normalAttribute = {};
	normalAttribute.binding = 0;
	normalAttribute.location = 1;
	normalAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
	normalAttribute.offset = offsetof(Vertex, Normal);

	//Color will be stored at Location 2
	VkVertexInputAttributeDescription colorAttribute = {};
	colorAttribute.binding = 0;
	colorAttribute.location = 2;
	colorAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
	colorAttribute.offset = offsetof(Vertex, Color);

	description.Attributes.push_back(positionAttribute);
	description.Attributes.push_back(normalAttribute);
	description.Attributes.push_back(colorAttribute);
	return description;
}

VkImageCreateInfo AllocatedImage::GetImageCreateInfo(VkFormat InFormat, VkImageUsageFlags InUsageFlags, VkExtent3D InExtent)
{
	VkImageCreateInfo CreateInfo{};

	CreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	CreateInfo.pNext = nullptr;

	CreateInfo.imageType = VK_IMAGE_TYPE_2D;
	//	Format holds what the data of the texture is, like holding a single float(for depth), or holding color.
	CreateInfo.format = InFormat;
	//	Extent is the size of the image, in pixels.
	CreateInfo.extent = InExtent;

	//	MipLevels holds the amount of mipmap levels the image has.
	//	TODO: Because we aren’t using them here, we leave the levels to 1.
	CreateInfo.mipLevels = 1;

	//	Array layers is for layered textures.
	//	You can create textures that are many - in - one, using layers. 
	//	An example of layered textures is cubemaps, where you have 6 layers, one layer for each face of the cubemap.
	//	We default it to 1 layer because we aren’t doing cubemaps.
	CreateInfo.arrayLayers = 1;

	//	Samples controls the MSAA behavior of the texture. This only makes sense for render targets, such as depth images and images you are rendering to. 
	//	TODO: We won’t be doing MSAA in this tutorial, so samples will be kept at 1 sample for the entire guide.
	CreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;

	//	Tiling is very important. Tiling describes how the data for the texture is arranged in the GPU. 
	//	For improved performance, GPUs do not store images as 2d arrays of pixels, but instead use complex custom formats, unique to the GPU brand and even models. 
	//	VK_IMAGE_TILING_OPTIMAL tells Vulkan to let the driver decide how the GPU arranges the memory of the image. 
	//	If you use VK_IMAGE_TILING_OPTIMAL, it won’t be possible to read the data from CPU or to write it without changing its tiling first 
	//	(it’s possible to change the tiling of a texture at any point, but this can be a costly operation). 
	//	The other tiling we can care about is VK_IMAGE_TILING_LINEAR, which will store the image as a 2d array of pixels. 
	//	While LINEAR tiling will be a lot slower, it will allow the cpu to safely write and read from that memory.
	CreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	
	//	This will control how the GPU handles the image memory.
	CreateInfo.usage = InUsageFlags;

	return CreateInfo;
}

VkImageViewCreateInfo AllocatedImage::GetImageViewCreateInfo(VkFormat InFormat, VkImage InImage, VkImageAspectFlagBits InAspectFlags)
{
	//	While imageType held the dimensionality of the texture, viewType has a lot more options, like VK_IMAGE_VIEW_TYPE_CUBE for cubemaps. 
	VkImageViewCreateInfo CreateInfo{};

	CreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	CreateInfo.pNext = nullptr;

	//	TODO: In here, we will have it matched to GetImageCreateInfo, and hardcode it to 2D images as it’s the most common case.
	CreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;

	//	Image has to point to the image this imageview is being created from. 
	//	As imageViews “wrap” an image, you need to point to the original one. 
	//	Format has to match the format in the image this view was created from. 
	CreateInfo.image = InImage;
	CreateInfo.format = InFormat;

	//	subresourceRange holds the information about where the image points to. 
	//	This is used for layered images, where you might have multiple layers in one image, and want to create an imageview that points to a specific layer. 
	//	It’s also possible to control the mipmap levels with it. 
	CreateInfo.subresourceRange.baseMipLevel = 0;
	CreateInfo.subresourceRange.levelCount = 1;
	CreateInfo.subresourceRange.baseArrayLayer = 0;
	CreateInfo.subresourceRange.layerCount = 1;
	
	//	aspectMask is similar to the usageFlags from the image. It’s about what this image is used for.
	CreateInfo.subresourceRange.aspectMask = InAspectFlags;

	return CreateInfo;
}

bool Zn::Vk::RawTexture::create_texture_image(String path, RawTexture& outTexture)
{
	outTexture.data = stbi_load(path.c_str(), &outTexture.width, &outTexture.height, &outTexture.channels, STBI_rgb_alpha);

	const bool success = outTexture.data != nullptr;

	if (success)
	{
		outTexture.size = outTexture.width * outTexture.height * 4;
	}

	return success;
}

void Zn::Vk::RawTexture::destroy_image(RawTexture& inTexture)
{
	if (inTexture.data != nullptr)
	{
		stbi_image_free(inTexture.data);
	}

	memset(&inTexture, 0, sizeof(inTexture));
}
