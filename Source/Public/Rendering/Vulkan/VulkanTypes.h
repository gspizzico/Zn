#pragma once

#include <optional>
#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>
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

		struct RawTexture
		{
			i32 width;
			i32 height;
			i32 channels;
			i32 size;
			u8* data;

			static bool create_texture_image(String path, RawTexture& outTexture);
			static void destroy_image(RawTexture& inTexture);
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

		struct alignas(16) DirectionalLight
		{
			glm::vec4 direction;
			glm::vec4 color;
			f32 intensity;
		};

		struct alignas(16) PointLight
		{
			glm::vec4 position;
			glm::vec4 color;
			f32 intensity;
			f32 constant_attenuation;
			f32 linear_attenuation;
			f32 quadratic_attenuation;
		};

		struct alignas(16) AmbientLight
		{
			glm::vec4 color;
			f32 intensity;
		};

		static constexpr u32 MAX_POINT_LIGHTS = 16;
		static constexpr u32 MAX_DIRECTIONAL_LIGHTS = 1;

		struct LightingUniforms
		{	
			PointLight point_lights[MAX_POINT_LIGHTS];
			DirectionalLight directional_lights[MAX_DIRECTIONAL_LIGHTS];
			AmbientLight ambient_light;
			u32 num_point_lights;
			u32 num_directional_lights;
		};
	}
}
