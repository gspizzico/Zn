#include <Importers/MeshImporter.h>
#include <Importers/TextureImporter.h>
#include <Rendering/RHI/RHIMesh.h>
#include <Math/Math.h>

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

i32 FetchIndex(const u8* src_, u32 componentType_)
{
    if (componentType_ == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
    {
        return static_cast<i32>(*src_);
    }
    else if (componentType_ == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
    {
        return static_cast<i32>(*reinterpret_cast<const u16*>(src_));
    }
    else if (componentType_ == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
    {
        return static_cast<i32>(*reinterpret_cast<const u32*>(src_));
    }
    else
    {
        check(false && "Index type not supported.");
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

Zn::AlphaMode TranslateAlphaMode(const Zn::String& gltfAlphaMode_)
{
    if (gltfAlphaMode_ == "OPAQUE")
    {
        return Zn::AlphaMode::Opaque;
    }
    else if (gltfAlphaMode_ == "MASK")
    {
        return Zn::AlphaMode::Mask;
    }
    else if (gltfAlphaMode_ == "BLEND")
    {
        return Zn::AlphaMode::Blend;
    }

    return Zn::AlphaMode::COUNT;
}

SharedPtr<TextureSource> CreateTextureSource(const tinygltf::Image& gltfImage_)
{
    check(gltfImage_.pixel_type == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE && "Missing implementation for other texture component types.");

    return SharedPtr<TextureSource>(new TextureSource {
        .width    = gltfImage_.width,
        .height   = gltfImage_.height,
        .channels = gltfImage_.component,
        .data     = gltfImage_.image,
    });
}

SamplerWrap TranslateWrap(i32 wrap_)
{
    if (wrap_ == TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE)
    {
        return SamplerWrap::Clamp;
    }
    else if (wrap_ == TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT)
    {
        return SamplerWrap::Mirrored;
    }

    return SamplerWrap::Repeat;
}

TextureSampler CreateTextureSampler(const tinygltf::Sampler& gltfSampler_)
{
    TextureSampler sampler {};

    switch (gltfSampler_.minFilter)
    {
    case TINYGLTF_TEXTURE_FILTER_NEAREST:
        sampler.minification = SamplerFilter::Nearest;
        break;
    case TINYGLTF_TEXTURE_FILTER_LINEAR:
        sampler.minification = SamplerFilter::Linear;
        break;
    case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST:
    {
        sampler.minification = SamplerFilter::Linear;
        sampler.mipMap       = SamplerFilter::Nearest;
    }
    break;
    case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR:
    {
        sampler.minification = SamplerFilter::Linear;
        sampler.mipMap       = SamplerFilter::Linear;
    }
    break;
    case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR:
    {
        sampler.minification = SamplerFilter::Nearest;
        sampler.mipMap       = SamplerFilter::Linear;
    }
    break;
    case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST:
    {
        sampler.minification = SamplerFilter::Nearest;
        sampler.mipMap       = SamplerFilter::Nearest;
    }
    break;
    }

    sampler.magnification = gltfSampler_.magFilter == TINYGLTF_TEXTURE_FILTER_NEAREST ? SamplerFilter::Nearest : SamplerFilter::Linear;
    sampler.wrapUV[0]     = TranslateWrap(gltfSampler_.wrapS);
    sampler.wrapUV[1]     = TranslateWrap(gltfSampler_.wrapT);
    sampler.wrapUV[2]     = TranslateWrap(gltfSampler_.wrapS);

    return sampler;
}
bool AssignTexture(i32 textureIndex_, const tinygltf::Model& model_, MeshImporterOutput& output_, ResourceHandle& outHandle_)
{
    if (textureIndex_ >= 0 && textureIndex_ < model_.textures.size())
    {
        const tinygltf::Texture& texture = model_.textures[textureIndex_];
        if (texture.source >= 0 && texture.source < model_.images.size())
        {
            const tinygltf::Image& image = model_.images[texture.source];

            if (!output_.textures.contains(image.uri))
            {
                output_.textures.insert({image.uri, CreateTextureSource(image)});
            }

            if (!output_.samplers.contains(image.uri))
            {
                tinygltf::Sampler sampler = texture.sampler >= 0 ? model_.samplers[texture.sampler] : tinygltf::Sampler();
                output_.samplers.insert({image.uri, CreateTextureSampler(sampler)});
            }

            // TODO: ResourceHandle should only be the pointer to the GPU resource. Using it here so that we know what to
            // associate.
            outHandle_ = ResourceHandle(HashCalculate(image.uri));

            return true;
        }
    }

    return false;
}

template<typename T>
const T* AccessBuffer(const tinygltf::Model& model_, const tinygltf::Accessor& accessor_, i32* outSize_ = nullptr)
{
    const tinygltf::BufferView& bufferView = model_.bufferViews[accessor_.bufferView];
    const tinygltf::Buffer&     buffer     = model_.buffers[bufferView.buffer];

    if (outSize_)
    {
        *outSize_ = accessor_.count * tinygltf::GetComponentSizeInBytes(accessor_.componentType);
    }

    return reinterpret_cast<const T*>(buffer.data.data() + bufferView.byteOffset + accessor_.byteOffset);
}
} // namespace

bool Zn::MeshImporter::ImportAll_GLTF(const String& fileName_, MeshImporterOutput& output_)
{
    ZN_TRACE_QUICKSCOPE();

    using namespace tinygltf;

    Model    model;
    TinyGLTF loader;

    String error;
    String warning;

    bool result = loader.LoadASCIIFromFile(&model, &error, &warning, fileName_);

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
        ZN_LOG(LogMeshImporter, ELogVerbosity::Error, "Failed to parse GLTF %s", fileName_.c_str());

        return false;
    }

    for (const Node& node : model.nodes)
    {
        if (node.mesh >= 0)
        {
            const Mesh& mesh = model.meshes[node.mesh];

            for (const Primitive& primitive : mesh.primitives)
            {
                RHIPrimitive& newPrimitive = output_.primitives.emplace_back(RHIPrimitive {});

                newPrimitive.matrix = glm::mat4 {1.f};

                if (node.translation.size() == 3)
                {
                    newPrimitive.matrix = glm::translate(newPrimitive.matrix, glm::vec3(glm::make_vec3(node.translation.data())));
                }
                if (node.rotation.size() == 4)
                {
                    glm::quat q = glm::make_quat(node.rotation.data());
                    newPrimitive.matrix *= glm::mat4(q);
                }
                if (node.scale.size() == 3)
                {
                    newPrimitive.matrix = glm::scale(newPrimitive.matrix, glm::vec3(glm::make_vec3(node.scale.data())));
                }
                if (node.matrix.size() == 16)
                {
                    newPrimitive.matrix = glm::make_mat4x4(node.matrix.data());
                }

                newPrimitive.topology = CastTopology(primitive.mode);

                if (auto positionAttribute = primitive.attributes.find("POSITION"); positionAttribute != primitive.attributes.end())
                {
                    const Accessor& accessor = model.accessors[positionAttribute->second];

                    const glm::vec3* src = AccessBuffer<glm::vec3>(model, accessor);

                    newPrimitive.position.assign(src, src + accessor.count);
                }
                else
                {
                    ZN_LOG(
                        LogMeshImporter, ELogVerbosity::Error, "Unable to find POSITION attribute for mesh in file %s", fileName_.c_str());
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

                    const glm::vec4* src = AccessBuffer<glm::vec4>(model, accessor);

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

                        const f32 upper = static_cast<f32>(accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT
                                                               ? std::numeric_limits<u16>::max()
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

                        const f32 upper = static_cast<f32>(accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT
                                                               ? std::numeric_limits<u16>::max()
                                                               : std::numeric_limits<u8>::max());

                        const f32 lower = 0.f;

                        for (i32 index = 0; index < accessor.count; ++index)
                        {
                            f32 r = static_cast<float>(*src);
                            f32 g = static_cast<float>(*(src + componentSize));
                            f32 b = static_cast<float>(*(src + componentSize * 2));
                            f32 a = isVec4 ? static_cast<float>(*(src + componentSize * 3)) : upper;

                            newPrimitive.color[index] = glm::vec4 {Math::MapRange(r, lower, upper, minR, maxR),
                                                                   Math::MapRange(g, lower, upper, minG, maxG),
                                                                   Math::MapRange(b, lower, upper, minB, maxB),
                                                                   Math::MapRange(a, lower, upper, minA, maxA)};

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

                    materialAttributes.baseColor =
                        *reinterpret_cast<const glm::dvec4*>(material.pbrMetallicRoughness.baseColorFactor.data());
                    materialAttributes.metalness   = material.pbrMetallicRoughness.metallicFactor;
                    materialAttributes.roughness   = material.pbrMetallicRoughness.roughnessFactor;
                    materialAttributes.emissive    = *reinterpret_cast<const glm::dvec3*>(material.emissiveFactor.data());
                    materialAttributes.occlusion   = material.occlusionTexture.strength;
                    materialAttributes.alphaCutoff = material.alphaCutoff;
                    materialAttributes.doubleSided = material.doubleSided;
                    materialAttributes.alphaMode   = TranslateAlphaMode(material.alphaMode);

                    // TODO: ResourceHandle should only be the pointer to the GPU resource. Using it here so that we know what to
                    // associate.
                    AssignTexture(
                        material.pbrMetallicRoughness.baseColorTexture.index, model, output_, materialAttributes.baseColorTexture);
                    AssignTexture(
                        material.pbrMetallicRoughness.metallicRoughnessTexture.index, model, output_, materialAttributes.metalnessTexture);
                    AssignTexture(material.normalTexture.index, model, output_, materialAttributes.normalTexture);
                    AssignTexture(material.occlusionTexture.index, model, output_, materialAttributes.occlusionTexture);
                    AssignTexture(material.emissiveTexture.index, model, output_, materialAttributes.emissiveTexture);
                }
            }
        }
    }

    if (output_.primitives.size() == 0)
    {
        ZN_LOG(LogMeshImporter, ELogVerbosity::Error, "Failed to initialize RHI Mesh from file: %s", fileName_.c_str());
        return false;
    }
    else
    {
        return true;
    }
}
