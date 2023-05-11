#include <Znpch.h>
#include <Core/Containers/Map.h>
#include <Engine/Importer/MeshImporter.h>
#include <Rendering/RHI/RHIMesh.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

using namespace Zn;

DEFINE_STATIC_LOG_CATEGORY(LogMeshImporter, ELogVerbosity::Log);

bool Zn::MeshImporter::Import(const String& fileName, RHIMesh& mesh)
{
    ZN_TRACE_QUICKSCOPE();

    // https://vkguide.dev/docs/chapter-3/obj_loading/

    tinyobj::ObjReaderConfig readerConfig {};
    readerConfig.mtl_search_path = "./";

    tinyobj::ObjReader reader;
    if (!reader.ParseFromFile(fileName, readerConfig))
    {
        if (!reader.Error().empty())
        {
            ZN_LOG(LogMeshImporter, ELogVerbosity::Error, reader.Error().c_str());
        }
        return false;
    }

    if (!reader.Warning().empty())
    {
        ZN_LOG(LogMeshImporter, ELogVerbosity::Warning, reader.Warning().c_str());
    }

    const tinyobj::attrib_t&                attrib    = reader.GetAttrib();
    const std::vector<tinyobj::shape_t>&    shapes    = reader.GetShapes();
    const std::vector<tinyobj::material_t>& materials = reader.GetMaterials();

    // Hardcoding triangle loading
    // If you use this code with a model that hasn’t been triangulated, you will have issues.
    static constexpr i32 kNumVertices = 3;

    UnorderedMap<u64, sizet> computedVertices {};

    for (const tinyobj::shape_t& shape : shapes)
    {
        mesh.vertices.reserve(mesh.vertices.size() + shape.mesh.indices.size());

        for (const tinyobj::index_t& index : shape.mesh.indices)
        {
            RHIVertex vertex {
                .position = glm::vec3(
                    attrib.vertices[kNumVertices * index.vertex_index + 0], attrib.vertices[kNumVertices * index.vertex_index + 1],
                    attrib.vertices[kNumVertices * index.vertex_index + 2]),
                .normal = glm::vec3(
                    attrib.normals[kNumVertices * index.normal_index + 0], attrib.normals[kNumVertices * index.normal_index + 1],
                    attrib.normals[kNumVertices * index.normal_index + 2]),
                .uv = glm::vec2 {attrib.texcoords[2 * index.texcoord_index + 0], 1.f - attrib.texcoords[2 * index.texcoord_index + 1]}};

            // TODO: Setting vertex color as vertex normal (display purposes)
            vertex.color = vertex.normal;

            auto result = computedVertices.insert({HashCalculate(vertex), mesh.vertices.size()});
            if (result.second)
            {
                mesh.vertices.emplace_back(std::move(vertex));
            }

            mesh.indices.emplace_back(result.first->second);
        }
    }

    return true;
}