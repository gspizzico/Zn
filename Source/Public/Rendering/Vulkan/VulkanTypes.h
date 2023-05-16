#pragma once

#include <optional>
#include <Rendering/RHI/Vulkan/Vulkan.h>
#include <Rendering/RHI/RHITypes.h>
#include <Core/Containers/Map.h>

namespace Zn
{
struct RHIMesh;
struct RHIPrimitiveGPU;
struct Material;

struct QueueFamilyIndices
{
    std::optional<uint32> Graphics;
    std::optional<uint32> Present;
};

struct SwapChainDetails
{
    vk::SurfaceCapabilitiesKHR   Capabilities;
    Vector<vk::SurfaceFormatKHR> Formats;
    Vector<vk::PresentModeKHR>   PresentModes;
};

struct MeshPushConstants
{
    glm::mat4 model;
    glm::mat4 model_inverse;
};

struct RenderObject
{
    RHIPrimitiveGPU* primitive;
    Material*        material;
    glm::vec3        location;
    glm::quat        rotation;
    glm::vec3        scale;
};

struct GPUCameraData
{
    glm::vec4 position;
    glm::mat4 view;
    glm::mat4 projection;
    glm::mat4 view_projection;
};

struct alignas(16) DirectionalLight
{
    glm::vec4 direction;
    glm::vec4 color;
    f32       intensity;
};

struct alignas(16) PointLight
{
    glm::vec4 position;
    glm::vec4 color;
    f32       intensity;
    f32       constant_attenuation;
    f32       linear_attenuation;
    f32       quadratic_attenuation;
};

struct alignas(16) AmbientLight
{
    glm::vec4 color;
    f32       intensity;
};

static constexpr u32 MAX_POINT_LIGHTS       = 16;
static constexpr u32 MAX_DIRECTIONAL_LIGHTS = 1;

struct LightingUniforms
{
    PointLight       point_lights[MAX_POINT_LIGHTS];
    DirectionalLight directional_lights[MAX_DIRECTIONAL_LIGHTS];
    AmbientLight     ambient_light;
    u32              num_point_lights;
    u32              num_directional_lights;
};
} // namespace Zn
