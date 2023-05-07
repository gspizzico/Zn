#include <Znpch.h>
#include <Rendering/Vulkan/VulkanTypes.h>


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