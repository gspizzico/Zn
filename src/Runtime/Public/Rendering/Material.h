#pragma once
#include <Core/CoreTypes.h>
#include <Core/Containers/Map.h>
#include <Rendering/RHI/RHI.h>

namespace Zn
{
struct Material
{
    vk::Pipeline       pipeline;
    vk::PipelineLayout layout;

    vk::ShaderModule vertexShader;
    vk::ShaderModule fragmentShader;

    // Textures
    Map<String, vk::ImageView> textureImageViews;
    Map<String, vk::Sampler>   textureSamplers;

    // Material parameters
    Map<String, f32> parameters;

    vk::DescriptorSet textureSet;

    // TODO: maybe this shouldn't be stored here
    vk::DescriptorSetLayout materialSetLayout;
};
} // namespace Zn
