#include <Znpch.h>
#include <Engine/Importer/MeshImporter.h>
#include <Engine/Importer/TextureImporter.h>
#include <Rendering/RHI/RHIMesh.h>
#include <Core/Math/Math.h>

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

#pragma warning(push)
#pragma warning(disable : 4018) // signed/unsigned mismatch
#include "tiny_gltf.h"
#pragma warning(pop)

namespace
{
using namespace Zn;

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

Zn::PrimitiveTopology CastTopology(i32 gltfTopology)
{
    if (gltfTopology == TINYGLTF_MODE_POINTS)
        return Zn::PrimitiveTopology::Points;
    if (gltfTopology == TINYGLTF_MODE_LINE)
        return Zn::PrimitiveTopology::Line;
    if (gltfTopology == TINYGLTF_MODE_LINE_LOOP)
        return Zn::PrimitiveTopology::LineLoop;
    if (gltfTopology == TINYGLTF_MODE_LINE_STRIP)
        return Zn::PrimitiveTopology::LineStrip;
    if (gltfTopology == TINYGLTF_MODE_TRIANGLES)
        return Zn::PrimitiveTopology::Triangles;
    if (gltfTopology == TINYGLTF_MODE_TRIANGLE_STRIP)
        return Zn::PrimitiveTopology::TriangleStrip;
    if (gltfTopology == TINYGLTF_MODE_TRIANGLE_FAN)
        return Zn::PrimitiveTopology::TriangleFan;

    return Zn::PrimitiveTopology::COUNT;
}

Zn::AlphaMode TranslateAlphaMode(const Zn::String& gltfAlphaMode)
{
    if (gltfAlphaMode == "OPAQUE")
    {
        return Zn::AlphaMode::Opaque;
    }
    else if (gltfAlphaMode == "MASK")
    {
        return Zn::AlphaMode::Mask;
    }
    else if (gltfAlphaMode == "BLEND")
    {
        return Zn::AlphaMode::Blend;
    }

    return Zn::AlphaMode::COUNT;
}

SharedPtr<TextureSource> CreateTextureSource(const tinygltf::Image& gltfImage)
{
    _ASSERT(gltfImage.pixel_type == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE && "Missing implementation for other texture component types.");

    return SharedPtr<TextureSource>(new TextureSource {
        .width    = gltfImage.width,
        .height   = gltfImage.height,
        .channels = gltfImage.component,
        .data     = gltfImage.image,
    });
}

bool AssignTexture(i32 textureIndex, const tinygltf::Model& model, MeshImporterOutput& output, ResourceHandle& outHandle)
{
    if (textureIndex >= 0 && textureIndex < model.textures.size())
    {
        const tinygltf::Texture& texture = model.textures[textureIndex];
        if (texture.source >= 0 && texture.source < model.images.size())
        {
            const tinygltf::Image& image = model.images[texture.source];

            if (!output.textures.contains(image.uri))
            {
                output.textures.insert({image.uri, CreateTextureSource(image)});
            }

            // TODO: ResourceHandle should only be the pointer to the GPU resource. Using it here so that we know what to
            // associate.
            outHandle = ResourceHandle(HashCalculate(image.uri));

            return true;
        }
    }

    return false;
}

template<typename T>
const T* AccessBuffer(const tinygltf::Model& model, const tinygltf::Accessor& accessor, i32* outSize = nullptr)
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
            const Primitive& primitive = gltfMesh.primitives[0];

            if (auto positionAttribute = primitive.attributes.find("POSITION"); positionAttribute != primitive.attributes.end())
            {
                const Accessor& accessor = model.accessors[positionAttribute->second];

                const glm::vec3* src = AccessBuffer<glm::vec3>(model, accessor);

                mesh.vertices.resize(accessor.count);

                memset(mesh.vertices.data(), 0, accessor.count * sizeof(RHIVertex));

                for (i32 index = 0; index < accessor.count; ++index, ++src)
                {
                    RHIVertex& vertex = mesh.vertices[index];
                    vertex.position   = *src;
                }
            }
            else
            {
                ZN_LOG(LogMeshImporter, ELogVerbosity::Error, "Unable to find POSITION attribute for mesh in file %s", fileName.c_str());
                return false;
            }

            if (auto normalAttribute = primitive.attributes.find("NORMAL"); normalAttribute != primitive.attributes.end())
            {
                const Accessor& accessor = model.accessors[normalAttribute->second];

                const glm::vec3* src = AccessBuffer<glm::vec3>(model, accessor);

                for (i32 index = 0; index < accessor.count; ++index, ++src)
                {
                    RHIVertex& vertex = mesh.vertices[index];
                    vertex.normal     = *src;
                }
            }

#if 0
            if (auto tangentAttribute = primitive.attributes.find("TANGENT"); tangentAttribute != primitive.attributes.end())
            {
                const Accessor& accessor = model.accessors[tangentAttribute->second];

                const glm::vec3* src = AccessBuffer<glm::vec3>(model, accessor);

                for (i32 index = 0; index < accessor.count; ++index, ++src)
                {
                }
            }
#endif
            if (auto texCoordAttribute = primitive.attributes.find("TEXCOORD_0"); texCoordAttribute != primitive.attributes.end())
            {
                const Accessor& accessor = model.accessors[texCoordAttribute->second];

                if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
                {
                    const glm::vec2* src = AccessBuffer<glm::vec2>(model, accessor);

                    for (i32 index = 0; index < accessor.count; ++index, ++src)
                    {
                        RHIVertex& vertex = mesh.vertices[index];
                        vertex.uv         = *src;
                    }
                }
                else
                {
                    const u8* src = AccessBuffer<u8>(model, accessor);

                    const f32 minX = static_cast<f32>(accessor.minValues[0]);
                    const f32 minY = static_cast<f32>(accessor.minValues[1]);

                    const f32 maxX = static_cast<f32>(accessor.maxValues[0]);
                    const f32 maxY = static_cast<f32>(accessor.maxValues[1]);

                    const i32 componentSize = GetComponentSizeInBytes(accessor.componentType);
                    const i32 stride        = componentSize * 2;

                    const f32 upper = static_cast<f32>(
                        accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT ? std::numeric_limits<u16>::max()
                                                                                         : std::numeric_limits<u8>::max());

                    const f32 lower = 0.f;

                    for (i32 index = 0; index < accessor.count; ++index)
                    {
                        RHIVertex& vertex = mesh.vertices[index];

                        f32 x = static_cast<float>(*src);
                        f32 y = static_cast<float>(*(src + componentSize));

                        vertex.uv.x = Math::MapRange(x, lower, upper, minX, maxX);
                        vertex.uv.y = Math::MapRange(y, lower, upper, minY, maxY);

                        src += stride;
                    }
                }
            }

            // TODO: Alpha value is always skipped.
            static_assert(std::is_same_v<decltype(RHIVertex::color), glm::vec3>, "vertex color type unsupported by GLTF importer.");

            if (auto colorAttribute = primitive.attributes.find("COLOR_0"); colorAttribute != primitive.attributes.end())
            {
                const Accessor& accessor = model.accessors[colorAttribute->second];

                if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
                {
                    if (accessor.type == TINYGLTF_TYPE_VEC3)
                    {
                        const glm::vec3* src = AccessBuffer<glm::vec3>(model, accessor);

                        for (i32 index = 0; index < accessor.count; ++index, ++src)
                        {
                            RHIVertex& vertex = mesh.vertices[index];
                            vertex.color      = *src;
                        }
                    }
                    else if (accessor.type == TINYGLTF_TYPE_VEC4)
                    {
                        const glm::u8* src = AccessBuffer<u8>(model, accessor);

                        for (i32 index = 0; index < accessor.count; ++index)
                        {
                            RHIVertex& vertex = mesh.vertices[index];
                            vertex.color      = *reinterpret_cast<const glm::vec3*>(src);

                            src += sizeof(float) * 4;
                        }
                    }
                }
                else
                {
                    const u8* src = AccessBuffer<u8>(model, accessor);

                    const f32 minR = static_cast<f32>(accessor.minValues[0]);
                    const f32 minG = static_cast<f32>(accessor.minValues[1]);
                    const f32 minB = static_cast<f32>(accessor.minValues[2]);

                    const f32 maxR = static_cast<f32>(accessor.maxValues[0]);
                    const f32 maxG = static_cast<f32>(accessor.maxValues[1]);
                    const f32 maxB = static_cast<f32>(accessor.maxValues[2]);

                    const i32 componentSize = GetComponentSizeInBytes(accessor.componentType);
                    const i32 stride        = componentSize * 2;

                    const f32 upper = static_cast<f32>(
                        accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT ? std::numeric_limits<u16>::max()
                                                                                         : std::numeric_limits<u8>::max());

                    const f32 lower = 0.f;

                    for (i32 index = 0; index < accessor.count; ++index)
                    {
                        RHIVertex& vertex = mesh.vertices[index];

                        f32 r = static_cast<float>(*src);
                        f32 g = static_cast<float>(*(src + componentSize));
                        f32 b = static_cast<float>(*(src + componentSize * 2));

                        vertex.color.r = Math::MapRange(r, lower, upper, minR, maxR);
                        vertex.color.g = Math::MapRange(g, lower, upper, minG, maxG);
                        vertex.color.b = Math::MapRange(b, lower, upper, minB, maxB);

                        src += stride;
                    }
                }
            }

            if (primitive.indices >= 0)
            {
                const Accessor& indicesAccessor = model.accessors[primitive.indices];

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

bool Zn::MeshImporter::ImportAll_GLTF(const String& fileName, MeshImporterOutput& output)
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
        const Mesh& mesh = model.meshes[0];

        for (const Primitive& primitive : mesh.primitives)
        {
            RHIPrimitive& newPrimitive = output.primitives.emplace_back(RHIPrimitive {});
            newPrimitive.topology      = CastTopology(primitive.mode);

            if (auto positionAttribute = primitive.attributes.find("POSITION"); positionAttribute != primitive.attributes.end())
            {
                const Accessor& accessor = model.accessors[positionAttribute->second];

                const glm::vec3* src = AccessBuffer<glm::vec3>(model, accessor);

                newPrimitive.position.assign(src, src + accessor.count);
            }
            else
            {
                ZN_LOG(LogMeshImporter, ELogVerbosity::Error, "Unable to find POSITION attribute for mesh in file %s", fileName.c_str());
                return false;
            }

            if (auto normalAttribute = primitive.attributes.find("NORMAL"); normalAttribute != primitive.attributes.end())
            {
                const Accessor& accessor = model.accessors[normalAttribute->second];

                const glm::vec3* src = AccessBuffer<glm::vec3>(model, accessor);

                newPrimitive.normal.assign(src, src + accessor.count);
            }

            if (auto tangentAttribute = primitive.attributes.find("TANGENT"); tangentAttribute != primitive.attributes.end())
            {
                const Accessor& accessor = model.accessors[tangentAttribute->second];

                const glm::vec3* src = AccessBuffer<glm::vec3>(model, accessor);

                newPrimitive.tangent.assign(src, src + accessor.count);
            }

            if (auto texCoordAttribute = primitive.attributes.find("TEXCOORD_0"); texCoordAttribute != primitive.attributes.end())
            {
                const Accessor& accessor = model.accessors[texCoordAttribute->second];

                if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
                {
                    const glm::vec2* src = AccessBuffer<glm::vec2>(model, accessor);

                    newPrimitive.uv.assign(src, src + accessor.count);
                }
                else
                {
                    newPrimitive.uv.resize(accessor.count);

                    const u8* src = AccessBuffer<u8>(model, accessor);

                    const f32 minX = static_cast<f32>(accessor.minValues[0]);
                    const f32 minY = static_cast<f32>(accessor.minValues[1]);

                    const f32 maxX = static_cast<f32>(accessor.maxValues[0]);
                    const f32 maxY = static_cast<f32>(accessor.maxValues[1]);

                    const i32 componentSize = GetComponentSizeInBytes(accessor.componentType);
                    const i32 stride        = componentSize * 2;

                    const f32 upper = static_cast<f32>(
                        accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT ? std::numeric_limits<u16>::max()
                                                                                         : std::numeric_limits<u8>::max());

                    const f32 lower = 0.f;

                    for (i32 index = 0; index < accessor.count; ++index)
                    {
                        f32 x = static_cast<float>(*src);
                        f32 y = static_cast<float>(*(src + componentSize));

                        newPrimitive.uv[index] =
                            glm::vec2 {Math::MapRange(x, lower, upper, minX, maxX), Math::MapRange(y, lower, upper, minY, maxY)};

                        src += stride;
                    }
                }
            }

            if (auto colorAttribute = primitive.attributes.find("COLOR_0"); colorAttribute != primitive.attributes.end())
            {
                const Accessor& accessor = model.accessors[colorAttribute->second];

                if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
                {
                    if (accessor.type == TINYGLTF_TYPE_VEC3)
                    {
                        const glm::vec3* src = AccessBuffer<glm::vec3>(model, accessor);

                        newPrimitive.color.resize(accessor.count);

                        for (i32 index = 0; index < accessor.count; ++index, ++src)
                        {
                            newPrimitive.color[index] = glm::vec4(*src, 1.f);
                        }
                    }
                    else if (accessor.type == TINYGLTF_TYPE_VEC4)
                    {
                        const glm::vec4* src = AccessBuffer<glm::vec4>(model, accessor);

                        newPrimitive.color.assign(src, src + accessor.count);
                    }
                }
                else
                {
                    const bool isVec4 = accessor.type == TINYGLTF_TYPE_VEC4;

                    const u8* src = AccessBuffer<u8>(model, accessor);

                    const f32 minR = static_cast<f32>(accessor.minValues[0]);
                    const f32 minG = static_cast<f32>(accessor.minValues[1]);
                    const f32 minB = static_cast<f32>(accessor.minValues[2]);
                    const f32 minA = isVec4 ? static_cast<f32>(accessor.minValues[3]) : 0.f;

                    const f32 maxR = static_cast<f32>(accessor.maxValues[0]);
                    const f32 maxG = static_cast<f32>(accessor.maxValues[1]);
                    const f32 maxB = static_cast<f32>(accessor.maxValues[2]);
                    const f32 maxA = isVec4 ? static_cast<f32>(accessor.maxValues[3]) : 1.f;

                    const i32 componentSize = GetComponentSizeInBytes(accessor.componentType);
                    const i32 stride        = componentSize * (isVec4 ? 4 : 3);

                    const f32 upper = static_cast<f32>(
                        accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT ? std::numeric_limits<u16>::max()
                                                                                         : std::numeric_limits<u8>::max());

                    const f32 lower = 0.f;

                    for (i32 index = 0; index < accessor.count; ++index)
                    {
                        f32 r = static_cast<float>(*src);
                        f32 g = static_cast<float>(*(src + componentSize));
                        f32 b = static_cast<float>(*(src + componentSize * 2));
                        f32 a = isVec4 ? static_cast<float>(*(src + componentSize * 3)) : upper;

                        newPrimitive.color[index] = glm::vec4 {
                            Math::MapRange(r, lower, upper, minR, maxR), Math::MapRange(g, lower, upper, minG, maxG),
                            Math::MapRange(b, lower, upper, minB, maxB), Math::MapRange(a, lower, upper, minA, maxA)};

                        src += stride;
                    }
                }
            }

            if (primitive.indices >= 0)
            {
                const Accessor& indicesAccessor = model.accessors[primitive.indices];

                if (indicesAccessor.type == TINYGLTF_TYPE_SCALAR)
                {
                    newPrimitive.indices.resize(indicesAccessor.count);

                    u32 componentSize = GetComponentSizeInBytes(indicesAccessor.componentType);

                    const u8* src = AccessBuffer<u8>(model, indicesAccessor);

                    for (i32 index = 0; index < indicesAccessor.count; ++index)
                    {
                        newPrimitive.indices[index] = FetchIndex(src, indicesAccessor.componentType);

                        src += componentSize;
                    }
                }
            }

            if (primitive.material >= 0 && primitive.material < model.materials.size())
            {
                const tinygltf::Material& material = model.materials[primitive.material];

                MaterialAttributes& materialAttributes = newPrimitive.materialAttributes;

                materialAttributes.baseColor   = *reinterpret_cast<const glm::dvec4*>(material.pbrMetallicRoughness.baseColorFactor.data());
                materialAttributes.metalness   = material.pbrMetallicRoughness.metallicFactor;
                materialAttributes.roughness   = material.pbrMetallicRoughness.roughnessFactor;
                materialAttributes.emissive    = *reinterpret_cast<const glm::dvec3*>(material.emissiveFactor.data());
                materialAttributes.alphaCutoff = material.alphaCutoff;
                materialAttributes.doubleSided = material.doubleSided;
                materialAttributes.alphaMode   = TranslateAlphaMode(material.alphaMode);

                // TODO: ResourceHandle should only be the pointer to the GPU resource. Using it here so that we know what to
                // associate.
                AssignTexture(material.pbrMetallicRoughness.baseColorTexture.index, model, output, materialAttributes.baseColorTexture);
                AssignTexture(
                    material.pbrMetallicRoughness.metallicRoughnessTexture.index, model, output, materialAttributes.metalnessTexture);
                AssignTexture(material.normalTexture.index, model, output, materialAttributes.normalTexture);
                AssignTexture(material.occlusionTexture.index, model, output, materialAttributes.occlusionTexture);
                AssignTexture(material.emissiveTexture.index, model, output, materialAttributes.emissiveTexture);
            }

            return true;
        }
        ZN_LOG(LogMeshImporter, ELogVerbosity::Error, "Failed to initialize RHI Mesh from file: %s", fileName.c_str());
        return false;
    }
}
