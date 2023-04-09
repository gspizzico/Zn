#include <Znpch.h>
#include <Rendering/Vulkan/VulkanMesh.h>

#define ZN_WITH_TINYOBJ 1

#if ZN_WITH_TINYOBJ
#include <tiny_obj_loader.cc>

DEFINE_STATIC_LOG_CATEGORY(LogTinyObj, ELogVerbosity::Log);

#endif

namespace Zn::Vk::Obj
{
	bool LoadMesh(String InFilename, Mesh& OutMesh)
	{
		// https://vkguide.dev/docs/chapter-3/obj_loading/
#if ZN_WITH_TINYOBJ		
		tinyobj::attrib_t VertexAttributes{};

		std::vector<tinyobj::shape_t> Shapes{};
		std::vector<tinyobj::material_t> Materials{};

		String Warning;
		String Error;

		tinyobj::LoadObj(&VertexAttributes, &Shapes, &Materials, &Warning, &Error, InFilename.c_str(), nullptr);

		if (!Warning.empty())
		{
			ZN_LOG(LogTinyObj, ELogVerbosity::Warning, Warning.c_str());
		}

		if (!Error.empty())
		{
			ZN_LOG(LogTinyObj, ELogVerbosity::Error, Error.c_str());
			return false;
		}


		// TinyObj Conversion Loop
		// Original at: https://github.com/tinyobjloader/tinyobjloade
		for (size_t iShapes = 0; iShapes < Shapes.size(); ++iShapes)
		{
			size_t IndexOffset = 0;

			const tinyobj::shape_t& CurrentShape = Shapes[iShapes];

			for (size_t iFaces = 0; iFaces < CurrentShape.mesh.num_face_vertices.size(); ++iFaces)
			{
				// Hardcoding triangle loading
				// If you use this code with a model that hasn’t been triangulated, you will have issues. 
				static const int32 NumVertices = 3;

				for (size_t iVertex = 0; iVertex < NumVertices; ++iVertex)
				{
					tinyobj::index_t IndexData = CurrentShape.mesh.indices[IndexOffset + iVertex];

					// Vertex Position

					Vertex NewVertex{};
					NewVertex.Position.x = VertexAttributes.vertices[NumVertices * IndexData.vertex_index + 0];
					NewVertex.Position.y = VertexAttributes.vertices[NumVertices * IndexData.vertex_index + 1];
					NewVertex.Position.z = VertexAttributes.vertices[NumVertices * IndexData.vertex_index + 2];

					// Vertex Normal
					NewVertex.Normal.x = VertexAttributes.normals[NumVertices * IndexData.normal_index + 0];
					NewVertex.Normal.y = VertexAttributes.normals[NumVertices * IndexData.normal_index + 1];
					NewVertex.Normal.z = VertexAttributes.normals[NumVertices * IndexData.normal_index + 2];

					// TODO: Setting vertex color as vertex normal (display purposes)

					NewVertex.Color = NewVertex.Normal;

					OutMesh.Vertices.emplace_back(std::move(NewVertex));
				}

				IndexOffset += NumVertices;
			}
		}

		return true;
#endif
	}
}
