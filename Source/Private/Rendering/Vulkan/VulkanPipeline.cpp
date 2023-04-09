#include <Znpch.h>
#include <Rendering/Vulkan/VulkanPipeline.h>
#include <Rendering/Vulkan/VulkanTypes.h>

using namespace Zn;

VkPipelineShaderStageCreateInfo VulkanPipeline::CreateShaderStage(VkShaderStageFlagBits InStageFlags, VkShaderModule InShaderModule)
{
	VkPipelineShaderStageCreateInfo CreateInfo{};
	CreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;

	//shader stage
	CreateInfo.stage = InStageFlags;
	//module containing the code for this shader stage
	CreateInfo.module = InShaderModule;
	//the entry point of the shader
	CreateInfo.pName = "main";
	return CreateInfo;
}

VkPipelineVertexInputStateCreateInfo VulkanPipeline::CreateVertexInputState()
{
	VkPipelineVertexInputStateCreateInfo CreateInfo{};
	CreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	//no vertex bindings or attributes
	CreateInfo.vertexBindingDescriptionCount = 0;
	CreateInfo.vertexAttributeDescriptionCount = 0;
	return CreateInfo;
}

VkPipelineInputAssemblyStateCreateInfo VulkanPipeline::CreateInputAssembly(VkPrimitiveTopology InTopology)
{
	VkPipelineInputAssemblyStateCreateInfo CreateInfo{};
	CreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;

	CreateInfo.topology = InTopology;
	//we are not going to use primitive restart on the entire tutorial so leave it on false
	CreateInfo.primitiveRestartEnable = VK_FALSE;
	return CreateInfo;
}

VkPipelineRasterizationStateCreateInfo VulkanPipeline::CreateRasterization(VkPolygonMode InPolygonMode)
{
	VkPipelineRasterizationStateCreateInfo CreateInfo{};
	CreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;

	CreateInfo.depthClampEnable = VK_FALSE;
	//discards all primitives before the rasterization stage if enabled which we don't want
	CreateInfo.rasterizerDiscardEnable = VK_FALSE;

	CreateInfo.polygonMode = InPolygonMode;
	CreateInfo.lineWidth = 1.0f;
	//no backface cull
	CreateInfo.cullMode = VK_CULL_MODE_NONE; // VK_CULL_MODE_BACK_BIT
	CreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
	//no depth bias
	CreateInfo.depthBiasEnable = VK_FALSE;
	CreateInfo.depthBiasConstantFactor = 0.0f;
	CreateInfo.depthBiasClamp = 0.0f;
	CreateInfo.depthBiasSlopeFactor = 0.0f;

	return CreateInfo;
}

VkPipelineMultisampleStateCreateInfo VulkanPipeline::CreateMSAA()
{
	VkPipelineMultisampleStateCreateInfo CreateInfo{};
	CreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;

	CreateInfo.sampleShadingEnable = VK_FALSE;
	//multisampling defaulted to no multisampling (1 sample per pixel)
	CreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	CreateInfo.minSampleShading = 1.0f;
	CreateInfo.pSampleMask = nullptr;
	CreateInfo.alphaToCoverageEnable = VK_FALSE;
	CreateInfo.alphaToOneEnable = VK_FALSE;
	return CreateInfo;
}

VkPipelineColorBlendAttachmentState VulkanPipeline::CreateColorBlendAttachmentState()
{
	VkPipelineColorBlendAttachmentState BlendAttachment{};
	BlendAttachment.colorWriteMask = (  VK_COLOR_COMPONENT_R_BIT 
									  | VK_COLOR_COMPONENT_G_BIT 
									  |	VK_COLOR_COMPONENT_B_BIT 
									  | VK_COLOR_COMPONENT_A_BIT
									 );

	BlendAttachment.blendEnable = VK_FALSE;
	return BlendAttachment;
}

VkPipelineDepthStencilStateCreateInfo VulkanPipeline::CreateDepthStencil(bool InDepthTest, bool InDepthWrite, VkCompareOp InCompareOp)
{
	VkPipelineDepthStencilStateCreateInfo CreateInfo{};
	CreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	CreateInfo.pNext = nullptr;

	//	Enable/Disable z-culling
	CreateInfo.depthTestEnable = InDepthTest ? VK_TRUE : VK_FALSE;
	//	Actually write to depth buffer. Usually == DepthTest, but sometimes you might want to not read from d-buffer for special effects.
	CreateInfo.depthWriteEnable = InDepthWrite ? VK_TRUE : VK_FALSE;
	//	VK_COMPARE_OP_ALWAYS does not do any depth test at all.
	//	VK_COMPARE_OP_LESS = Draw if Z < whatever is on d-buffer
	//	VK_COMPARE_OP_EQUAL = Draw if depth matches what is on d-buffer.
	CreateInfo.depthCompareOp = InDepthTest ? InCompareOp : VK_COMPARE_OP_ALWAYS;

	CreateInfo.depthBoundsTestEnable = VK_FALSE;
	//	Min and Max depth bounds lets us cap the depth test. 
	//	If the depth is outside of bounds, the pixel will be skipped. 
	CreateInfo.minDepthBounds = 0.f;	// Optional
	CreateInfo.maxDepthBounds = 1.f;	// Optional
	
	//	We won’t be using stencil test, so that’s set to VK_FALSE by default.
	CreateInfo.stencilTestEnable = VK_FALSE;

	return CreateInfo;
}

VkPipeline VulkanPipeline::NewVkPipeline(VkDevice InDevice, VkRenderPass InRenderPass,
										VkShaderModule InVertexShader, VkShaderModule InFragmentShader,
										VkExtent2D InSwapChainExtent, VkPipelineLayout InLayout,
										const Vk::VertexInputDescription& InVertexInputDescription)
{
	static const Vector<VkDynamicState> DynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

	// This allow us to change viewport/scissor when we draw instead of baking it into the pipeline.
	VkPipelineDynamicStateCreateInfo DynamicStateCreateInfo{};
	DynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	DynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(DynamicStates.size());
	DynamicStateCreateInfo.pDynamicStates = DynamicStates.data();

	VkPipelineShaderStageCreateInfo ShaderStages[] = { CreateShaderStage(VK_SHADER_STAGE_VERTEX_BIT, InVertexShader), CreateShaderStage(VK_SHADER_STAGE_FRAGMENT_BIT, InFragmentShader)};

	VkPipelineVertexInputStateCreateInfo VertexInput = CreateVertexInputState();

	const uint32 NumBindings = static_cast<uint32>(InVertexInputDescription.Bindings.size());
	const uint32 NumAttributes = static_cast<uint32>(InVertexInputDescription.Attributes.size());

	if (NumBindings > 0 && NumAttributes > 0)
	{
		VertexInput.vertexBindingDescriptionCount = NumBindings;
		VertexInput.pVertexBindingDescriptions = InVertexInputDescription.Bindings.data();

		VertexInput.vertexAttributeDescriptionCount = NumAttributes;
		VertexInput.pVertexAttributeDescriptions = InVertexInputDescription.Attributes.data();
		VertexInput.flags = InVertexInputDescription.Flags;
	}

	VkPipelineInputAssemblyStateCreateInfo InputAssembly = CreateInputAssembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

	VkViewport Viewport{};
	Viewport.x = 0;
	Viewport.y = 0;
	Viewport.width = (float) InSwapChainExtent.width;
	Viewport.height = (float) InSwapChainExtent.height;
	Viewport.minDepth = 0.0f;
	Viewport.maxDepth = 1.0f;

	VkRect2D Scissors{};
	Scissors.offset = { 0,0 };
	Scissors.extent = InSwapChainExtent;

	VkPipelineViewportStateCreateInfo ViewportState{};
	ViewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	ViewportState.viewportCount = 1;
	ViewportState.scissorCount = 1;
	ViewportState.pViewports = &Viewport;
	ViewportState.pScissors = &Scissors;


	VkPipelineRasterizationStateCreateInfo Rasterizer = CreateRasterization(VK_POLYGON_MODE_FILL);
	
	VkPipelineMultisampleStateCreateInfo MSAA = CreateMSAA();

	VkPipelineColorBlendAttachmentState BlendAttachment = CreateColorBlendAttachmentState();

	//	setup dummy color blending. We aren't using transparent objects yet
	//	the blending is just "no blend", but we do write to the color attachment
	VkPipelineColorBlendStateCreateInfo ColorBlending{};
	ColorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;

	ColorBlending.logicOpEnable = VK_FALSE;
	ColorBlending.logicOp = VK_LOGIC_OP_COPY;
	ColorBlending.attachmentCount = 1;
	ColorBlending.pAttachments = &BlendAttachment;

	VkPipelineDepthStencilStateCreateInfo DepthStencil = CreateDepthStencil(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);

	VkGraphicsPipelineCreateInfo PipelineInfo = {};
	PipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	
	PipelineInfo.stageCount = static_cast<uint32>(std::size(ShaderStages));
	PipelineInfo.pStages = ShaderStages;
	PipelineInfo.pVertexInputState = &VertexInput;
	PipelineInfo.pInputAssemblyState = &InputAssembly;
	PipelineInfo.pViewportState = &ViewportState;
	PipelineInfo.pRasterizationState = &Rasterizer;
	PipelineInfo.pMultisampleState = &MSAA;
	PipelineInfo.pColorBlendState = &ColorBlending;
	PipelineInfo.pDepthStencilState = &DepthStencil;
	PipelineInfo.layout = InLayout;
	PipelineInfo.renderPass = InRenderPass;
	PipelineInfo.subpass = 0;
	PipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

	//it's easy to error out on create graphics pipeline, so we handle it a bit better than the common VK_CHECK case
	VkPipeline OutVkPipeline;
	if (vkCreateGraphicsPipelines(
		InDevice, VK_NULL_HANDLE, 1, &PipelineInfo, nullptr, &OutVkPipeline) != VK_SUCCESS)
	{	
		return VK_NULL_HANDLE; // failed to create graphics pipeline
	}
	else
	{
		return OutVkPipeline;
	}

}
