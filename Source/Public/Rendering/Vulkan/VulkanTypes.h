#pragma once

#include <optional>
#include <Rendering/RHI/Vulkan/Vulkan.h>
#include <Rendering/RHI/RHITypes.h>
#include <Core/Containers/Map.h>

namespace Zn
{
	struct RHIMesh;
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

		struct VertexInputDescription
		{
			Vector<vk::VertexInputBindingDescription> Bindings{};
			Vector<vk::VertexInputAttributeDescription> Attributes{};
			vk::PipelineVertexInputStateCreateFlags Flags{ 0 };
		};

		struct MeshPushConstants
		{
			glm::vec4 Data;
			glm::mat4 RenderMatrix;
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
			RHIMesh* mesh;
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
