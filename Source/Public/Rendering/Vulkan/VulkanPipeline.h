#pragma once
#include <vulkan/vulkan.h>

namespace Zn
{
	class VulkanPipeline
	{
	public:

		static VkPipelineShaderStageCreateInfo CreateShaderStage(VkShaderStageFlagBits InStageFlags, VkShaderModule InShaderModule);
		
		static VkPipelineVertexInputStateCreateInfo CreateVertexInputState();
		
		static VkPipelineInputAssemblyStateCreateInfo CreateInputAssembly(VkPrimitiveTopology InTopology);
		
		static VkPipelineRasterizationStateCreateInfo CreateRasterization(VkPolygonMode InPolygonMode);
		
		static VkPipelineMultisampleStateCreateInfo CreateMSAA();
		
		static VkPipelineColorBlendAttachmentState CreateColorBlendAttachmentState();

		static VkPipeline NewVkPipeline(VkDevice InDevice, VkRenderPass InRenderPass, VkShaderModule InVertexShader, VkShaderModule InFragmentShader, VkExtent2D InSwapChainExtent, VkPipelineLayout InLayout);

	};
}
