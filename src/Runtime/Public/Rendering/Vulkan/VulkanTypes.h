#pragma once

#include <optional>
#include <glm/glm.hpp>
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
};

struct GPUCameraData
{
    glm::vec4 position;
    glm::mat4 view;
    glm::mat4 projection;
    glm::mat4 view_projection;
};

struct UBOLights
{
    glm::vec4 position;
    float     intensity;
    float     range;
    float     unused0;
    float     unused1;
};
} // namespace Zn
