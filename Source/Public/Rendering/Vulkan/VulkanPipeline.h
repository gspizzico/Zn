#pragma once
#include <Rendering/Vulkan/VulkanTypes.h>

namespace Zn
{
struct RHIInputLayout;

class VulkanPipeline
{
  public:
    static const RHIInputLayout defaultInputLayout;
    static const RHIInputLayout defaultIndirectInputLayout;

    static vk::PipelineShaderStageCreateInfo CreateShaderStage(vk::ShaderStageFlagBits stageFlags, vk::ShaderModule shaderModule);

    static vk::PipelineInputAssemblyStateCreateInfo CreateInputAssembly(vk::PrimitiveTopology topology);

    static vk::PipelineRasterizationStateCreateInfo CreateRasterization(vk::PolygonMode polygonMode);

    static vk::PipelineMultisampleStateCreateInfo CreateMSAA();

    static vk::PipelineColorBlendAttachmentState CreateColorBlendAttachmentState();

    static vk::PipelineDepthStencilStateCreateInfo CreateDepthStencil(bool depthTest, bool depthWrite, vk::CompareOp compareOp);

    static vk::Pipeline NewVkPipeline(
        vk::Device device, vk::RenderPass renderPass, vk::ShaderModule vertexShader, vk::ShaderModule fragmentShader, vk::Extent2D swapChainExtent,
        vk::PipelineLayout pipelineLayout, const RHIInputLayout& inputLayout = defaultInputLayout);
};
} // namespace Zn
