#include <Znpch.h>
#include <Engine/Importer/MeshImporter.h>
#include <Rendering/RHI/RHIMesh.h>

#define TINYGLTF_IMPLEMENTATION

#ifndef STB_IMAGE_IMPLEMENTATION
    #define STB_IMAGE_IMPLEMENTATION
#endif

#define STBI_WRITE_NO_STDIO

#ifndef STB_IMAGE_WRITE_IMPLEMENTATION
    #define STB_IMAGE_WRITE_IMPLEMENTATION
#endif

#define TINYGLTF_NOEXCEPTION
#define JSON_NOEXCEPTION

#include "tiny_gltf.h"

namespace
{
i32 FetchIndex(const u8* src, u32 componentType)
{
    if (componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
    {
        return static_cast<i32>(*src);
    }
    else if (componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
    {
        return static_cast<i32>(*reinterpret_cast<const u16*>(src));
    }
    else if (componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
    {
        return static_cast<i32>(*reinterpret_cast<const u32*>(src));
    }
    else
    {
        _ASSERT(false && "Index type not supported.");
        return 0;
    }
}

template<typename T> const T* AccessBuffer(const tinygltf::Model& model, const tinygltf::Accessor& accessor, i32* outSize = nullptr)
{
    const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
    const tinygltf::Buffer&     buffer     = model.buffers[bufferView.buffer];

    if (outSize)
    {
        *outSize = accessor.count * tinygltf::GetComponentSizeInBytes(accessor.componentType);
    }

    return reinterpret_cast<const T*>(buffer.data.data() + bufferView.byteOffset + accessor.byteOffset);
}
} // namespace

bool Zn::MeshImporter::Import_GLTF(const String& fileName, RHIMesh& mesh)
{
    ZN_TRACE_QUICKSCOPE();

    using namespace tinygltf;

    Model    model;
    TinyGLTF loader;

    String error;
    String warning;

    bool result = loader.LoadASCIIFromFile(&model, &error, &warning, fileName);

    if (!warning.empty())
    {
        ZN_LOG(LogMeshImporter, ELogVerbosity::Warning, warning.c_str());
    }

    if (!error.empty())
    {
        ZN_LOG(LogMeshImporter, ELogVerbosity::Error, error.c_str());
    }

    if (!result)
    {
        ZN_LOG(LogMeshImporter, ELogVerbosity::Error, "Failed to parse GLTF %s", fileName.c_str());

        return false;
    }

    // TODO: Handling single mesh
    if (model.meshes.size() == 1)
    {
        const Mesh& gltfMesh = model.meshes[0];

        // TODO: Handling single primitive
        if (gltfMesh.primitives.size() == 1)
        {
            const Primitive& gltfPrimitive            = gltfMesh.primitives[0];
            i32              indicesAccessorIndex     = gltfPrimitive.indices;
            i32              verticesPositionAccessor = gltfPrimitive.attributes.find("POSITION")->second;
            i32              verticesNormalAccessor   = gltfPrimitive.attributes.find("NORMAL")->second;

            const Accessor& positionAccessor = model.accessors[verticesPositionAccessor];

            if (positionAccessor.type == TINYGLTF_TYPE_VEC3)
            {
                const glm::vec3* src = AccessBuffer<glm::vec3>(model, positionAccessor);
                mesh.vertices.resize(positionAccessor.count);

                memset(mesh.vertices.data(), 0, positionAccessor.count * sizeof(RHIVertex));

                for (i32 index = 0; index < positionAccessor.count; ++index, ++src)
                {
                    RHIVertex& vertex = mesh.vertices[index];
                    vertex.position   = *src;
                }
            }

            const Accessor& normalAccessor = model.accessors[verticesNormalAccessor];

            if (normalAccessor.type == TINYGLTF_TYPE_VEC3)
            {
                const glm::vec3* src = AccessBuffer<glm::vec3>(model, normalAccessor);

                for (i32 index = 0; index < normalAccessor.count; ++index, ++src)
                {
                    RHIVertex& vertex = mesh.vertices[index];
                    vertex.normal     = *src;
                }
            }

            if (indicesAccessorIndex >= 0)
            {
                const Accessor& indicesAccessor = model.accessors[indicesAccessorIndex];

                if (indicesAccessor.type == TINYGLTF_TYPE_SCALAR)
                {
                    mesh.indices.resize(indicesAccessor.count);

                    u32 componentSize = GetComponentSizeInBytes(indicesAccessor.componentType);

                    const u8* src = AccessBuffer<u8>(model, indicesAccessor);

                    for (i32 index = 0; index < indicesAccessor.count; ++index)
                    {
                        mesh.indices[index] = FetchIndex(src, indicesAccessor.componentType);

                        src += componentSize;
                    }
                }
            }

            return true;
        }
    }

    ZN_LOG(LogMeshImporter, ELogVerbosity::Error, "Failed to initialize RHI Mesh from file: %s", fileName.c_str());
    return false;
}
