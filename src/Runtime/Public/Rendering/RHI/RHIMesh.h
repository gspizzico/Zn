#pragma once

#include <Core/CoreTypes.h>
#include <glm/glm.hpp>
#include <Rendering/RHI/RHI.h>
#include <Rendering/RHI/RHITypes.h>
#include <Rendering/RHI/RHIVertex.h>
#include <Core/Containers/StaticPool.h>

namespace Zn
{
enum class PrimitiveTopology : u8
{
    Points,
    Line,
    LineLoop,
    LineStrip,
    Triangles,
    TriangleStrip,
    TriangleFan,
    COUNT
};

enum class AlphaMode : u8
{
    Opaque,
    Mask,
    Blend,
    COUNT
};

struct RHIMesh
{
    Vector<RHIVertex> vertices;
    Vector<i32>       indices;

    RHIBuffer vertexBuffer;
    RHIBuffer indexBuffer;
};

// Based on gltf format.
struct MaterialAttributes
{
    glm::vec4 baseColor   = glm::vec4(0.f);
    f32       metalness   = 0.f;
    f32       roughness   = 0.f;
    glm::vec3 emissive    = glm::vec3(0.f);
    f32       occlusion   = 1.f;
    f32       alphaCutoff = 0.5f;
    AlphaMode alphaMode   = AlphaMode::Opaque;
    bool      doubleSided = false;
    // TODO: Move texture name outside mat attributes
    String    baseColorTexture {};
    String    metalnessTexture {};
    String    normalTexture {};
    String    occlusionTexture {};
    String    emissiveTexture {};

    DefaultPoolHandle baseColorTextureHandle {};
    DefaultPoolHandle metalnessTextureHandle {};
    DefaultPoolHandle normalTextureHandle {};
    DefaultPoolHandle occlusionTextureHandle {};
    DefaultPoolHandle emissiveTextureHandle {};
};

struct alignas(16) UBOMaterialAttributes
{
    glm::vec4 baseColor   = glm::vec4(0.f);
    f32       metalness   = 0.f;
    f32       roughness   = 0.f;
    f32       alphaCutoff = 0.5;
    glm::vec3 emissive    = glm::vec3(0.f);
    f32       occlusion   = 1.f;
    f32       padding0;
};

enum class SamplerFilter : u8
{
    None,
    Nearest,
    Linear,
    COUNT
};

enum class SamplerWrap : u8
{
    Repeat,
    Clamp,
    Mirrored,
    COUNT
};

struct TextureSampler
{
    SamplerFilter minification;
    SamplerFilter magnification;

    // TODO:
    SamplerFilter mipMap;
    SamplerWrap   wrapUV[3];
};

struct RHIPrimitive
{
    glm::mat4          matrix;
    Vector<glm::vec3>  position;
    Vector<glm::vec3>  normal;
    Vector<glm::vec4>  tangent;
    Vector<glm::vec2>  uv;
    Vector<glm::vec4>  color;
    Vector<u32>        indices;
    PrimitiveTopology  topology;
    MaterialAttributes materialAttributes;
};

struct RHIPrimitiveGPU
{
    glm::mat4          matrix;
    RHIBuffer          position;
    RHIBuffer          normal;
    RHIBuffer          tangent;
    RHIBuffer          uv;
    RHIBuffer          color;
    RHIBuffer          indices;
    u32                numVertices;
    u32                numIndices;
    MaterialAttributes materialAttributes;
    RHIBuffer          uboMaterialAttributes;
};
} // namespace Zn
