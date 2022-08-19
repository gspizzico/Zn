#pragma once

namespace Zn
{
	namespace Vk
	{
		struct AllocatedBuffer;

		struct VertexInputDescription
		{
			Vector<VkVertexInputBindingDescription> Bindings{};
			Vector<VkVertexInputAttributeDescription> Attributes{};
			VkPipelineVertexInputStateCreateFlags Flags{0};
		};

		struct Vertex
		{
			float Position[3];
			float Normal[3];
			float Color[3];

			static VertexInputDescription GetVertexInputDescription();
		};

		struct Mesh
		{
			Vector<Vertex> Vertices;

			AllocatedBuffer Buffer;
		};
	}

}
