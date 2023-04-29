#pragma once

#include <optional>
#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>
#include <glm/glm.hpp>
#include <Core/Containers/Map.h>

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

		struct AllocatedImage
		{
			VkImage Image;
			VmaAllocation Allocation;

			//	TODO: MOVE AWAY FROM HERE
			static VkImageCreateInfo GetImageCreateInfo(VkFormat InFormat, VkImageUsageFlags InUsageFlags, VkExtent3D InExtent);
			//	TODO: MOVE AWAY FROM HERE
			static VkImageViewCreateInfo GetImageViewCreateInfo(VkFormat InFormat, VkImage InImage, VkImageAspectFlagBits InAspectFlags);
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

		struct Mesh
		{
			Vector<Vertex> Vertices;

			AllocatedBuffer Buffer;
		};

		struct Material
		{
			VkPipeline pipeline;
			VkPipelineLayout layout;

			VkShaderModule vertex_shader;
			VkShaderModule fragment_shader;

			// Textures
			UnorderedMap<String, VkImageView> texture_image_views;
			UnorderedMap<String, VkSampler> texture_samplers;

			// Material parameters
			UnorderedMap<String, f32> parameters;
		};

		struct RenderObject
		{
			Vk::Mesh* mesh;
			Vk::Material* material;
			glm::mat4 transform;
		};

		struct GPUCameraData
		{
			glm::mat4 view;
			glm::mat4 projection;
			glm::mat4 view_projection;
		};
	}
}
