#include <Znpch.h>
#include <Engine/Importer/MeshImporter.h>
#include <Rendering/RHI/RHIMesh.h>

#include <tiny_obj_loader.cc>

using namespace Zn;

DEFINE_STATIC_LOG_CATEGORY(LogMeshImporter, ELogVerbosity::Log);

bool Zn::MeshImporter::Import(const String& fileName, RHIMesh& mesh)
{
	ZN_TRACE_QUICKSCOPE();

	// https://vkguide.dev/docs/chapter-3/obj_loading/
	tinyobj::attrib_t vertexAttributes{};

	std::vector<tinyobj::shape_t> shapes{};
	std::vector<tinyobj::material_t> materials{};

	String warning;
	String error;

	tinyobj::LoadObj(&vertexAttributes, &shapes, &materials, &warning, &error, fileName.c_str(), nullptr);

	if (!warning.empty())
	{
		ZN_LOG(LogMeshImporter, ELogVerbosity::Warning, warning.c_str());
	}

	if (!error.empty())
	{
		ZN_LOG(LogMeshImporter, ELogVerbosity::Error, error.c_str());
		return false;
	}

	// TinyObj Conversion Loop
	// Original at: https://github.com/tinyobjloader/tinyobjloade
	for (size_t iShapes = 0; iShapes < shapes.size(); ++iShapes)
	{
		size_t indexOffset = 0;

		const tinyobj::shape_t& currentShape = shapes[iShapes];

		for (size_t iFaces = 0; iFaces < currentShape.mesh.num_face_vertices.size(); ++iFaces)
		{
			// Hardcoding triangle loading
			// If you use this code with a model that hasn’t been triangulated, you will have issues. 
			static const int32 numVertices = 3;

			for (size_t iVertex = 0; iVertex < numVertices; ++iVertex)
			{
				tinyobj::index_t indexData = currentShape.mesh.indices[indexOffset + iVertex];

				// Vertex Position

				RHIVertex vertex{};
				vertex.position.x = vertexAttributes.vertices[numVertices * indexData.vertex_index + 0];
				vertex.position.y = vertexAttributes.vertices[numVertices * indexData.vertex_index + 1];
				vertex.position.z = vertexAttributes.vertices[numVertices * indexData.vertex_index + 2];

				// Vertex Normal
				vertex.normal.x = vertexAttributes.normals[numVertices * indexData.normal_index + 0];
				vertex.normal.y = vertexAttributes.normals[numVertices * indexData.normal_index + 1];
				vertex.normal.z = vertexAttributes.normals[numVertices * indexData.normal_index + 2];

				vertex.uv.x = vertexAttributes.texcoords[2 * indexData.texcoord_index + 0];
				vertex.uv.y = 1.f - vertexAttributes.texcoords[2 * indexData.texcoord_index + 1];

				// TODO: Setting vertex color as vertex normal (display purposes)

				vertex.color = vertex.normal;

				mesh.vertices.emplace_back(std::move(vertex));
			}

			indexOffset += numVertices;
		}
	}

	return true;
}