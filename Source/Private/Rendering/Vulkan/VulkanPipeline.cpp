#include <Znpch.h>
#include <Rendering/Vulkan/VulkanPipeline.h>
#include <Rendering/RHI/RHI.h>
#include <Rendering/RHI/RHIVertex.h>
#include <Rendering/RHI/RHIInstanceData.h>
#include <Rendering/RHI/RHIInputLayout.h>

using namespace Zn;

const RHIInputLayout VulkanPipeline::defaultInputLayout = {
    .bindings   = {{
          .binding   = 0,
          .stride    = sizeof(RHIVertex),
          .inputRate = vk::VertexInputRate::eVertex,
    }},
    .attributes = {
        {.location = 0, .binding = 0, .format = vk::Format::eR32G32B32A32Sfloat, .offset = offsetof(RHIVertex, position)},
        {.location = 1, .binding = 0, .format = vk::Format::eR32G32B32A32Sfloat, .offset = offsetof(RHIVertex, normal)},
        {.location = 2, .binding = 0, .format = vk::Format::eR32G32B32A32Sfloat, .offset = offsetof(RHIVertex, color)},
        {.location = 3, .binding = 0, .format = vk::Format::eR32G32Sfloat, .offset = offsetof(RHIVertex, uv)},
    }};

const RHIInputLayout VulkanPipeline::gltfInputLayout = {.bindings   = {{
                                                                           .binding   = 0,
                                                                           .stride    = sizeof(glm::vec3),
                                                                           .inputRate = vk::VertexInputRate::eVertex,
                                                                     },
                                                                       {
                                                                           .binding   = 1,
                                                                           .stride    = sizeof(glm::vec3),
                                                                           .inputRate = vk::VertexInputRate::eVertex,
                                                                     },
                                                                      {
                                                                            .binding   = 2,
                                                                            .stride    = sizeof(glm::vec4),
                                                                            .inputRate = vk::VertexInputRate::eVertex,
                                                                      },
                                                                       {
                                                                           .binding   = 3,
                                                                           .stride    = sizeof(glm::vec2),
                                                                           .inputRate = vk::VertexInputRate::eVertex,
                                                                     },
                                                                      /* {
                                                                           .binding   = 4,
                                                                           .stride    = sizeof(glm::vec4),
                                                                           .inputRate = vk::VertexInputRate::eVertex,
                                                                     }*/},
                                                        .attributes = {{
                                                                           .location = 0,
                                                                           .binding  = 0,
                                                                           .format   = vk::Format::eR32G32B32Sfloat,
                                                                           .offset   = 0,
                                                                       },
                                                                       {
                                                                           .location = 1,
                                                                           .binding  = 1,
                                                                           .format   = vk::Format::eR32G32B32Sfloat,
                                                                           .offset   = 0,
                                                                       },
                                                                       {
                                                                           .location = 2,
                                                                           .binding  = 2,
                                                                           .format   = vk::Format::eR32G32B32A32Sfloat,
                                                                           .offset   = 0,
                                                                       },
                                                                       {
                                                                           .location = 3,
                                                                           .binding  = 3,
                                                                           .format   = vk::Format::eR32G32Sfloat,
                                                                           .offset   = 0,
                                                                       },
/*                                                                       {
                                                                           .location = 4,
                                                                           .binding  = 4,
                                                                           .format   = vk::Format::eR32G32B32A32Sfloat,
                                                                           .offset   = 0,
                                                                       }*/}};

const RHIInputLayout VulkanPipeline::defaultIndirectInputLayout = {
    .bindings =
        {
            vk::VertexInputBindingDescription {
                .binding   = 0,
                .stride    = sizeof(RHIVertex),
                .inputRate = vk::VertexInputRate::eVertex,
            },
            vk::VertexInputBindingDescription {
                .binding   = 1,
                .stride    = sizeof(RHIInstanceData),
                .inputRate = vk::VertexInputRate::eInstance,
            },
        },
    .attributes = {
        {.location = 0, .binding = 0, .format = vk::Format::eR32G32B32A32Sfloat, .offset = offsetof(RHIVertex, position)},
        {.location = 1, .binding = 0, .format = vk::Format::eR32G32B32A32Sfloat, .offset = offsetof(RHIVertex, normal)},
        {.location = 2, .binding = 0, .format = vk::Format::eR32G32B32A32Sfloat, .offset = offsetof(RHIVertex, color)},
        {.location = 3, .binding = 0, .format = vk::Format::eR32G32Sfloat, .offset = offsetof(RHIVertex, uv)},
        {.location = 4, .binding = 1, .format = vk::Format::eR32G32B32A32Sfloat, .offset = sizeof(glm::vec4) * 0},
        {.location = 5, .binding = 1, .format = vk::Format::eR32G32B32A32Sfloat, .offset = sizeof(glm::vec4) * 1},
        {.location = 6, .binding = 1, .format = vk::Format::eR32G32B32A32Sfloat, .offset = sizeof(glm::vec4) * 2},
        {.location = 7, .binding = 1, .format = vk::Format::eR32G32B32A32Sfloat, .offset = sizeof(glm::vec4) * 3},
    }};

vk::PipelineShaderStageCreateInfo VulkanPipeline::CreateShaderStage(vk::ShaderStageFlagBits stageFlags, vk::ShaderModule shaderModule)
{
    vk::PipelineShaderStageCreateInfo createInfo {};

    // shader stage
    createInfo.stage  = stageFlags;
    // module containing the code for this shader stage
    createInfo.module = shaderModule;
    // the entry point of the shader
    createInfo.pName  = "main";
    return createInfo;
}

vk::PipelineInputAssemblyStateCreateInfo VulkanPipeline::CreateInputAssembly(vk::PrimitiveTopology topology)
{
    vk::PipelineInputAssemblyStateCreateInfo createInfo {};

    createInfo.topology               = topology;
    // we are not going to use primitive restart on the entire tutorial so leave it on false
    createInfo.primitiveRestartEnable = false;
    return createInfo;
}

vk::PipelineRasterizationStateCreateInfo VulkanPipeline::CreateRasterization(vk::PolygonMode polygonMode)
{
    vk::PipelineRasterizationStateCreateInfo createInfo {};

    createInfo.depthClampEnable        = false;
    // discards all primitives before the rasterization stage if enabled which we don't want
    createInfo.rasterizerDiscardEnable = false;

    createInfo.polygonMode             = polygonMode;
    createInfo.lineWidth               = 1.0f;
    // no backface cull
    createInfo.cullMode                = vk::CullModeFlagBits::eNone; // VK_CULL_MODE_BACK_BIT
    createInfo.frontFace               = vk::FrontFace::eClockwise;
    // no depth bias
    createInfo.depthBiasEnable         = false;
    createInfo.depthBiasConstantFactor = 0.0f;
    createInfo.depthBiasClamp          = 0.0f;
    createInfo.depthBiasSlopeFactor    = 0.0f;

    return createInfo;
}

vk::PipelineMultisampleStateCreateInfo VulkanPipeline::CreateMSAA()
{
    vk::PipelineMultisampleStateCreateInfo createInfo {};

    createInfo.sampleShadingEnable   = false;
    // multisampling defaulted to no multisampling (1 sample per pixel)
    createInfo.rasterizationSamples  = vk::SampleCountFlagBits::e1;
    createInfo.minSampleShading      = 1.0f;
    createInfo.pSampleMask           = nullptr;
    createInfo.alphaToCoverageEnable = false;
    createInfo.alphaToOneEnable      = false;
    return createInfo;
}

vk::PipelineColorBlendAttachmentState VulkanPipeline::CreateColorBlendAttachmentState()
{
    vk::PipelineColorBlendAttachmentState blendAttachment {};
    blendAttachment.colorWriteMask =
        (vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);

    blendAttachment.blendEnable = false;
    return blendAttachment;
}

vk::PipelineDepthStencilStateCreateInfo VulkanPipeline::CreateDepthStencil(bool depthTest, bool depthWrite, vk::CompareOp compareOp)
{
    vk::PipelineDepthStencilStateCreateInfo createInfo {};

    //	Enable/Disable z-culling
    createInfo.depthTestEnable  = depthTest;
    //	Actually write to depth buffer. Usually == DepthTest, but sometimes you might want to not read from d-buffer for special effects.
    createInfo.depthWriteEnable = depthWrite;
    //	VK_COMPARE_OP_ALWAYS does not do any depth test at all.
    //	VK_COMPARE_OP_LESS = Draw if Z < whatever is on d-buffer
    //	VK_COMPARE_OP_EQUAL = Draw if depth matches what is on d-buffer.
    createInfo.depthCompareOp   = depthTest ? compareOp : vk::CompareOp::eAlways;

    createInfo.depthBoundsTestEnable = false;
    //	Min and Max depth bounds lets us cap the depth test.
    //	If the depth is outside of bounds, the pixel will be skipped.
    createInfo.minDepthBounds        = 0.f; // Optional
    createInfo.maxDepthBounds        = 1.f; // Optional

    //	We won’t be using stencil test, so that’s set to VK_FALSE by default.
    createInfo.stencilTestEnable = false;

    return createInfo;
}

vk::Pipeline VulkanPipeline::NewVkPipeline(vk::Device            device,
                                           vk::RenderPass        renderPass,
                                           vk::ShaderModule      vertexShader,
                                           vk::ShaderModule      fragmentShader,
                                           vk::Extent2D          swapChainExtent,
                                           vk::PipelineLayout    pipelineLayout,
                                           const RHIInputLayout& inputLayout)
{
    vk::PipelineShaderStageCreateInfo shaderStages[] = {
        CreateShaderStage(vk::ShaderStageFlagBits::eVertex, vertexShader),
        CreateShaderStage(vk::ShaderStageFlagBits::eFragment, fragmentShader),
    };

    vk::PipelineVertexInputStateCreateInfo vertexInput {};

    if (inputLayout.bindings.size() > 0 && inputLayout.attributes.size() > 0)
    {
        vertexInput.setVertexBindingDescriptions(inputLayout.bindings);
        vertexInput.setVertexAttributeDescriptions(inputLayout.attributes);
        vertexInput.flags = inputLayout.flags;
    }

    vk::PipelineInputAssemblyStateCreateInfo inputAssembly = CreateInputAssembly(vk::PrimitiveTopology::eTriangleList);

    vk::Viewport viewport {
        .x        = 0,
        .y        = 0,
        .width    = static_cast<f32>(swapChainExtent.width),
        .height   = static_cast<f32>(swapChainExtent.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };

    vk::Rect2D scissors {
        .offset = {.x = 0, .y = 0},
        .extent = swapChainExtent,
    };

    vk::PipelineViewportStateCreateInfo viewportState {
        .viewportCount = 1, .pViewports = &viewport, .scissorCount = 1, .pScissors = &scissors};

    vk::PipelineRasterizationStateCreateInfo rasterizer = CreateRasterization(vk::PolygonMode::eFill);

    vk::PipelineMultisampleStateCreateInfo msaa = CreateMSAA();

    vk::PipelineColorBlendAttachmentState blendAttachment = CreateColorBlendAttachmentState();

    //	setup dummy color blending. We aren't using transparent objects yet
    //	the blending is just "no blend", but we do write to the color attachment
    vk::PipelineColorBlendStateCreateInfo colorBlending {};

    colorBlending.logicOpEnable   = false;
    colorBlending.logicOp         = vk::LogicOp::eCopy;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments    = &blendAttachment;

    vk::PipelineDepthStencilStateCreateInfo depthStencil = CreateDepthStencil(true, true, vk::CompareOp::eLessOrEqual);

    static const vk::DynamicState dynamicStates[] = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};

    // This allow us to change viewport/scissor when we draw instead of baking it into the pipeline.
    vk::PipelineDynamicStateCreateInfo dynamicStateCreateInfo;
    dynamicStateCreateInfo.setDynamicStates(dynamicStates);

    vk::GraphicsPipelineCreateInfo pipelineInfo = {};

    pipelineInfo.setStages(shaderStages);
    pipelineInfo.pVertexInputState   = &vertexInput;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState      = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState   = &msaa;
    pipelineInfo.pColorBlendState    = &colorBlending;
    pipelineInfo.pDepthStencilState  = &depthStencil;
    pipelineInfo.layout              = pipelineLayout;
    pipelineInfo.renderPass          = renderPass;
    pipelineInfo.subpass             = 0;
    pipelineInfo.basePipelineHandle  = VK_NULL_HANDLE;
    pipelineInfo.pDynamicState       = &dynamicStateCreateInfo;

    // it's easy to error out on create graphics pipeline, so we handle it a bit better than the common VK_CHECK case

    vk::ResultValue<vk::Pipeline> result = device.createGraphicsPipeline({}, pipelineInfo);

    if (result.result == vk::Result::eSuccess)
    {
        return result.value;
    }
    else
    {
        return vk::Pipeline {};
    }
}
