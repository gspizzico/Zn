#pragma once

#include <optional>
#include <vulkan/vulkan.h>
#define VULKAN_HPP_NO_SPACESHIP_OPERATOR
#include <vulkan/vulkan.hpp>
//#include <vma/vk_mem_alloc.h>
#include <vk_mem_alloc.hpp>
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
			vk::SurfaceCapabilitiesKHR Capabilities;
			Vector<vk::SurfaceFormatKHR> Formats;
			Vector<vk::PresentModeKHR> PresentModes;
		};

		struct AllocatedBuffer
		{
			vk::Buffer Buffer;
			vma::Allocation Allocation;
		};

		struct AllocatedImage
		{
			vk::Image image;
			vk::ImageView imageView;
			vma::Allocation allocation;			

			//	TODO: MOVE AWAY FROM HERE
			static vk::ImageCreateInfo GetImageCreateInfo(vk::Format inFormat, vk::ImageUsageFlags inUsageFlags, vk::Extent3D inExtent);
			//	TODO: MOVE AWAY FROM HERE
			static vk::ImageViewCreateInfo GetImageViewCreateInfo(vk::Format InFormat, vk::Image InImage, vk::ImageAspectFlagBits InAspectFlags);
		};

		struct RawTexture
		{
			i32 width;
			i32 height;
			i32 channels;
			i32 size;
			u8* data;

			static bool LoadFromFile(String path, RawTexture& outTexture);
			static void Unload(RawTexture& inTexture);
		};

		struct VertexInputDescription
		{
			Vector<vk::VertexInputBindingDescription> Bindings{};
			Vector<vk::VertexInputAttributeDescription> Attributes{};
			vk::PipelineVertexInputStateCreateFlags Flags{ 0 };
		};

		struct Vertex
		{
			glm::vec3 Position;
			glm::vec3 Normal;
			glm::vec3 Color;
			glm::vec2 UV;

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
			vk::Pipeline pipeline;
			vk::PipelineLayout layout;

			vk::ShaderModule vertexShader;
			vk::ShaderModule fragmentShader;

			// Textures
			UnorderedMap<String, vk::ImageView> textureImageViews;
			UnorderedMap<String, vk::Sampler> textureSamplers;

			// Material parameters
			UnorderedMap<String, f32> parameters;


			vk::DescriptorSet textureSet;
		};

		struct RenderObject
		{
			Vk::Mesh* mesh;
			Vk::Material* material;
			glm::vec3 location;
			glm::quat rotation;
			glm::vec3 scale;
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
