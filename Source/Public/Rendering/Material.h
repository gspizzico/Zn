#pragma once
#include <Core/Types.h>
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
    UnorderedMap<String, vk::ImageView> textureImageViews;
    UnorderedMap<String, vk::Sampler>   textureSamplers;

    // Material parameters
    UnorderedMap<String, f32> parameters;

    vk::DescriptorSet textureSet;

    // TODO: maybe this shouldn't be stored here
    vk::DescriptorSetLayout materialSetLayout;
};
} // namespace Zn
