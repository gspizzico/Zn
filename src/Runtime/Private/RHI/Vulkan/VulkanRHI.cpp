#include <RHI/RHIDevice.h>
#include <RHI/Vulkan/Vulkan.h>
#include <RHI/Vulkan/VulkanRHI.h>
#include <RHI/Vulkan/VulkanDebug.h>
#include <RHI/Vulkan/VulkanPlatform.h>
#include <RHI/Vulkan/VulkanGPU.h>
#include <RHI/Vulkan/VulkanContext.h>
#include <RHI/Vulkan/VulkanDescriptor.h>
#include <RHI/RHIResourceBuffer.h>
#include <Core/CoreAssert.h>
#include <RHI/RHIResource.h>
#include <RHI/RHITexture.h>
#include <RHI/Vulkan/VulkanTexture.h>
#include <RHI/RHIBuffer.h>
#include <RHI/Vulkan/VulkanBuffer.h>
#include <RHI/Vulkan/VulkanResource.h>
#include <RHI/Vulkan/VulkanSwapChain.h>
#include <RHI/Vulkan/VulkanRenderPass.h>
#include <RHI/Vulkan/VulkanPipeline.h>
#include <Core/Misc/Hash.h>

#include <deque>

#define MAKE_RESOURCE_HANDLE(name) ResourceHandle(HashCalculate(name))

using namespace Zn;

namespace
{
class CleanupQueue
{
  public:
    ~CleanupQueue()
    {
        Flush();
    }

    void Push(std::function<void()>&& function_)
    {
        queue.emplace_back(std::move(function_));
    }

    void Flush()
    {
        while (!queue.empty())
        {
            std::invoke(queue.front());

            queue.pop_front();
        }
    }

  private:
    std::deque<std::function<void()>> queue;
};
static const cstring GDepthTextureName = "__DepthTexture";
static TextureHandle GDepthTextureHandle;

using TTextureResource = TResource<RHITexture, VulkanTexture>;
using TBufferResource  = TResource<RHIBuffer, VulkanBuffer>;

VulkanSwapChain                                                                         GSwapChain;
RHIResourceBuffer<TTextureResource, TextureHandle, u16_max>                             GTextures;
RHIResourceBuffer<VulkanRenderPass, RenderPassHandle, u8_max>                           GRenderPasses;
RHIResourceBuffer<vk::DescriptorPool, DescriptorPoolHandle, u8_max>                     GDescriptorPools;
RHIResourceBuffer<vk::DescriptorSetLayout, DescriptorSetLayoutHandle, u16_max>          GDescriptorSetLayouts;
RHIResourceBuffer<vk::ShaderModule, ShaderModuleHandle, u16_max>                        GShaders;
RHIResourceBuffer<std::pair<vk::PipelineLayout, vk::Pipeline>, PipelineHandle, u16_max> GPipelines;
RenderPassHandle                                                                        GMainRenderPass;
CleanupQueue                                                                            GCleanupQueue;

TTextureResource CreateRHITexture(const RHITextureDescriptor& descriptor_)
{
    vk::ImageCreateInfo createInfo {
        .imageType   = vk::ImageType::e2D,
        .format      = TranslateRHIFormat(descriptor_.format),
        .extent      = vk::Extent3D {descriptor_.width, descriptor_.height, 1},
        .mipLevels   = descriptor_.numMips,
        .arrayLayers = 1,
        .samples     = TranslateSampleCount(descriptor_.samples),
        .tiling      = vk::ImageTiling::eOptimal,
        .usage       = TranslateRHITextureFlags(descriptor_.flags),
    };

    //	VMA_MEMORY_USAGE_GPU_ONLY to make sure that the image is allocated on fast VRAM.
    //	To make absolutely sure that VMA really allocates the image into VRAM, we give it VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT on required
    //  flags. This forces VMA library to allocate the image on VRAM no matter what. (The Memory Usage part is more like a hint)

    vma::AllocationCreateInfo allocation {
        .usage         = vma::MemoryUsage::eGpuOnly,
        .requiredFlags = vk::MemoryPropertyFlagBits::eDeviceLocal,
    };

    TTextureResource texture = TTextureResource {RHITexture {
                                                     .width  = descriptor_.width,
                                                     .height = descriptor_.height,
                                                     .format = descriptor_.format,
                                                     .flags  = descriptor_.flags,
                                                 },
                                                 VulkanTexture {}};

    ZN_VK_CHECK(
        VulkanContext::Get().allocator.createImage(&createInfo, &allocation, &texture.payload.image, &texture.payload.memory, nullptr));

    return texture;
}

TBufferResource CreateBuffer(const RHIBufferDescriptor& descriptor_)
{
    vk::BufferCreateInfo createInfo {
        .size  = descriptor_.size,
        .usage = TranslateRHIBufferUsage(descriptor_.bufferUsage),
    };

    // TODO: hard-coded required flags
    vma::AllocationCreateInfo allocationInfo {
        .flags         = vma::AllocationCreateFlags {0},
        .usage         = TranslateRHIResourceUsage(descriptor_.memoryUsage),
        .requiredFlags = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
    };

    auto& allocator = VulkanContext::Get().allocator;
    check(allocator);

    auto [buffer, allocation] = allocator.createBuffer(createInfo, allocationInfo);

    return TBufferResource(
        RHIBuffer {
            .size        = descriptor_.size,
            .bufferUsage = descriptor_.bufferUsage,
        },
        VulkanBuffer {
            .data   = buffer,
            .memory = allocation,

        });
}

void CopyToGPU(vma::Allocation allocation_, void* src_, uint32 size_)
{
    auto& allocator = VulkanContext::Get().allocator;

    void* dst = allocator.mapMemory(allocation_);

    memcpy(dst, src_, size_);

    allocator.unmapMemory(allocation_);
}

void CreateFramebuffer(VulkanRenderPass& renderPass_)
{
    vk::ImageView depthTextureIV = GTextures[GDepthTextureHandle]->payload.imageView;

    auto frameBuffers = Span(renderPass_.frameBuffer);

    for (auto it = std::begin(frameBuffers); it != std::end(frameBuffers); ++it)
    {
        Span<const vk::ImageView> attachments = {GSwapChain.imageViews[it.Index()], depthTextureIV};

        // TODO: Layers?
        vk::FramebufferCreateInfo createInfo {
            .renderPass      = renderPass_.renderPass,
            .attachmentCount = static_cast<uint32>(attachments.Size()),
            .pAttachments    = attachments.Data(),
            .width           = GSwapChain.extent.width,
            .height          = GSwapChain.extent.height,
            .layers          = 1,
        };

        *it = VulkanContext::Get().device.createFramebuffer(createInfo);
    }
}
} // namespace

RHIDevice::RHIDevice()
{
    // Initialize vk::DynamicLoader - It's needed to call .dll functions.
    vk::DynamicLoader dynamicLoader;
    auto              pGetInstance = dynamicLoader.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
    VULKAN_HPP_DEFAULT_DISPATCHER.init(pGetInstance);

    vk::ApplicationInfo appInfo {
        .pApplicationName = "Zn",
        .pEngineName      = "Zn",
        .apiVersion       = VK_API_VERSION_1_3,
    };

    vk::InstanceCreateInfo instanceCreateInfo {.pApplicationInfo = &appInfo};

    Vector<cstring> instanceExtensions = PlatformVulkan::GetInstanceExtensions();

#if ZN_VK_VALIDATION_LAYERS
    VulkanValidation::InitializeInstanceForDebug(instanceCreateInfo, instanceExtensions);
#endif

    instanceCreateInfo.setPEnabledExtensionNames(instanceExtensions);

    VulkanContext& vkContext = VulkanContext::Get();
    vkContext.instance       = vk::createInstance(instanceCreateInfo);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(vkContext.instance);

#if ZN_VK_VALIDATION_LAYERS
    VulkanValidation::InitializeDebugMessenger(vkContext.instance);
#endif

    vkContext.surface = PlatformVulkan::CreateSurface(vkContext.instance);

    vkContext.gpu = VulkanGPU(vkContext.instance, vkContext.surface);

    // Logical Device

    vkContext.device = vkContext.gpu.CreateDevice();

    VULKAN_HPP_DEFAULT_DISPATCHER.init(vkContext.device);

    if (vkContext.gpu.graphicsQueue != u32_max)
    {
        vkContext.graphicsQueue = vkContext.device.getQueue(vkContext.gpu.graphicsQueue, 0);
    }

    if (vkContext.gpu.presentQueue != u32_max)
    {
        vkContext.presentQueue = vkContext.device.getQueue(vkContext.gpu.presentQueue, 0);
    }

    if (vkContext.gpu.computeQueue != u32_max)
    {
        vkContext.presentQueue = vkContext.device.getQueue(vkContext.gpu.computeQueue, 0);
    }

    if (vkContext.gpu.transferQueue != u32_max)
    {
        vkContext.transferQueue = vkContext.device.getQueue(vkContext.gpu.transferQueue, 0);
    }

    // Vma Allocator
    vma::AllocatorCreateInfo allocatorCreateInfo {
        .physicalDevice = vkContext.gpu.gpu, .device = vkContext.device, .instance = vkContext.instance};

    vkContext.allocator = vma::createAllocator(allocatorCreateInfo);

    CreateSwapChain();

    // Allocate command contexts

    // === GFX Context

    vkContext.graphicsCmdContext.commandPool = vkContext.device.createCommandPool(vk::CommandPoolCreateInfo {
        .flags            = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = vkContext.gpu.graphicsQueue,
    });

    {
        auto commandBuffers = vkContext.device.allocateCommandBuffers(vk::CommandBufferAllocateInfo {
            .commandPool        = vkContext.graphicsCmdContext.commandPool,
            .level              = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = kVkMaxImageCount,
        });

        memcpy(DataPtr(vkContext.graphicsCmdContext.commandBuffers),
               commandBuffers.data(),
               SizeOfArray(vkContext.graphicsCmdContext.commandBuffers));
    }

    GCleanupQueue.Push(
        []()
        {
            VulkanContext& vkContext = VulkanContext::Get();

            vkContext.device.destroyCommandPool(vkContext.graphicsCmdContext.commandPool);

            Memory::Memzero(vkContext.graphicsCmdContext);
        });

    // === Upload Context

    vkContext.uploadCmdContext.commandPool = vkContext.device.createCommandPool(vk::CommandPoolCreateInfo {
        .flags            = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = vkContext.gpu.transferQueue, // TODO: It was GFX queue previously, need to check for sync?
    });

    {
        auto commandBuffers = vkContext.device.allocateCommandBuffers(vk::CommandBufferAllocateInfo {
            .commandPool        = vkContext.uploadCmdContext.commandPool,
            .level              = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = SizeOf(vkContext.uploadCmdContext.commandBuffers),
        });

        memcpy(DataPtr(vkContext.uploadCmdContext.commandBuffers),
               commandBuffers.data(),
               SizeOfArray(vkContext.uploadCmdContext.commandBuffers));
    }

    vkContext.uploadFence = vkContext.device.createFence(vk::FenceCreateInfo {
        .flags = vk::FenceCreateFlags {0},
    });

    GCleanupQueue.Push(
        []()
        {
            VulkanContext& vkContext = VulkanContext::Get();

            vkContext.device.destroyCommandPool(vkContext.uploadCmdContext.commandPool);
            vkContext.device.destroyFence(vkContext.uploadFence);
            vkContext.uploadFence = VK_NULL_HANDLE;

            Memory::Memzero(vkContext.uploadCmdContext);
        });

    // Create Render Pass

    RHIRenderPassDescription renderPassDescription {
        .attachments =
            {
                {
                    .type          = AttachmentType::Color,
                    .format        = TranslateVkFormat(GSwapChain.surfaceFormat.format),
                    .samples       = SampleCount::s1,
                    .loadOp        = LoadOp::Clear,
                    .storeOp       = StoreOp::Store,
                    .initialLayout = ImageLayout::Undefined,
                    .finalLayout   = ImageLayout::Present,
                },
                {
                    .type          = AttachmentType::Depth,
                    .format        = GTextures[GDepthTextureHandle]->resource.format,
                    .samples       = SampleCount::s1,
                    .loadOp        = LoadOp::Clear,
                    .storeOp       = StoreOp::Store,
                    .initialLayout = ImageLayout::Undefined,
                    .finalLayout   = ImageLayout::DepthStencilAttachment,
                },
            },
        .subpasses    = {{
               .pipeline         = PipelineType::Graphics,
               .colorAttachments = {{
                   .attachment = 0,
                   .layout     = ImageLayout::ColorAttachment,
            }},
               .depthStencilAttachment =
                {
                       .attachment = 1,
                       .layout     = ImageLayout::DepthStencilAttachment,
                },
        }},
        .dependencies = {{
                             .srcSubpass    = SubpassDependency::kExternalSubpass,
                             .dstSubpass    = 0,
                             .srcStageMask  = PipelineStage::ColorAttachmentOutput,
                             .dstStageMask  = PipelineStage::ColorAttachmentOutput,
                             .srcAccessMask = AccessFlag::None,
                             .dstAccessMask = AccessFlag::ColorAttachmentWrite,
                         },
                         {
                             .srcSubpass    = SubpassDependency::kExternalSubpass,
                             .dstSubpass    = 0,
                             .srcStageMask  = PipelineStage::EarlyFragmentTests | PipelineStage::LateFragmentTests,
                             .dstStageMask  = PipelineStage::EarlyFragmentTests | PipelineStage::LateFragmentTests,
                             .srcAccessMask = AccessFlag::None,
                             .dstAccessMask = AccessFlag::DepthStencilAttachmentWrite,
                         }

        }};

    // TODO: Create RenderPass object that also contains FrameBuffer.
    GMainRenderPass = CreateRenderPass(renderPassDescription);

    vkContext.mainRenderPass = GRenderPasses[GMainRenderPass]->renderPass;
}

RHIDevice::~RHIDevice()
{
    VulkanContext& vkContext = VulkanContext::Get();

    if (!vkContext.instance)
    {
        return;
    }

    if (vkContext.device)
    {
        vkContext.device.waitIdle();
    }

    CleanupSwapChain();

    for (auto it = std::begin(GTextures); it != std::end(GTextures); ++it)
    {
        VulkanTexture& texture = it->payload;

        vkContext.device.destroyImageView(texture.imageView);
        vkContext.allocator.destroyImage(texture.image, texture.memory);
    }

    GTextures.Clear();

    for (VulkanRenderPass& renderPass : GRenderPasses)
    {
        vkContext.device.destroyRenderPass(renderPass.renderPass);
    }

    GRenderPasses.Clear();

    for (vk::DescriptorSetLayout& descriptorSetLayout : GDescriptorSetLayouts)
    {
        vkContext.device.destroyDescriptorSetLayout(descriptorSetLayout);
    }

    GDescriptorSetLayouts.Clear();

    for (vk::DescriptorPool& descriptorPool : GDescriptorPools)
    {
        vkContext.device.destroyDescriptorPool(descriptorPool);
    }

    GDescriptorPools.Clear();

    for (auto& [layout, pipeline] : GPipelines)
    {
        vkContext.device.destroyPipeline(pipeline);
        vkContext.device.destroyPipelineLayout(layout);
    }

    GPipelines.Clear();

    for (vk::ShaderModule& shaderModule : GShaders)
    {
        vkContext.device.destroyShaderModule(shaderModule);
    }

    GShaders.Clear();

    GCleanupQueue.Flush();

    if (vkContext.allocator)
    {
        vkContext.allocator.destroy();
        vkContext.allocator = nullptr;
    }

    if (vkContext.surface)
    {
        vkContext.instance.destroySurfaceKHR(vkContext.surface);
        vkContext.surface = nullptr;
    }

    if (vkContext.device)
    {
        vkContext.device.destroy();
        vkContext.device = nullptr;
    }

    vkContext.gpu = VulkanGPU();

#if ZN_VK_VALIDATION_LAYERS
    VulkanValidation::DeinitializeDebugMessenger(vkContext.instance);
#endif

    vkContext.instance.destroy();
    vkContext.instance = nullptr;
}

TextureHandle RHIDevice::CreateTexture(const RHITextureDescriptor& descriptor_)
{
    TTextureResource texture = CreateRHITexture(descriptor_);

    // const uint32 bufferSize = (descriptor_.width * descriptor_.height * descriptor_.numChannels * descriptor_.channelSize);

    // TBufferResource stagingBuffer = CreateBuffer(RHIBufferDescriptor {
    //     .size        = bufferSize,
    //     .bufferUsage = RHIBufferUsage::TransferSrc,
    //     .memoryUsage = RHIResourceUsage::Cpu,
    // });

    // CopyToGPU(stagingBuffer.payload.memory, descriptor_.data, bufferSize);

    TextureHandle resourceHandle = GTextures.Add(std::move(texture));

    return resourceHandle;
}

RenderPassHandle Zn::RHIDevice::CreateRenderPass(const RHIRenderPassDescription& description_)
{
    VulkanRenderPassDescription vkRenderPass = TranslateRenderPass(description_);

    vk::RenderPassCreateInfo createInfo {
        .attachmentCount = static_cast<uint32>(vkRenderPass.attachments.size()),
        .pAttachments    = vkRenderPass.attachments.data(),
        .subpassCount    = static_cast<uint32>(vkRenderPass.vkSubpasses.size()),
        .pSubpasses      = vkRenderPass.vkSubpasses.data(),
        .dependencyCount = static_cast<uint32>(vkRenderPass.dependencies.size()),
        .pDependencies   = vkRenderPass.dependencies.data(),
    };

    VulkanRenderPass renderPass;
    renderPass.renderPass = VulkanContext::Get().device.createRenderPass(createInfo);

    CreateFramebuffer(renderPass);

    return GRenderPasses.Add(std::move(renderPass));
}

DescriptorPoolHandle Zn::RHIDevice::CreateDescriptorPool(const RHI::DescriptorPoolDescription& description_)
{
    Vector<vk::DescriptorPoolSize> sizes;
    for (const auto& typeToSize : description_.desc)
    {
        sizes.push_back(vk::DescriptorPoolSize {
            .type            = RHI::TranslateDescriptorType(typeToSize.first),
            .descriptorCount = typeToSize.second,
        });
    }

    vk::DescriptorPoolCreateInfo createInfo {
        .flags   = RHI::TranslateDescriptorPoolCreateFlags(description_.flags),
        .maxSets = description_.maxSets,
    };

    createInfo.setPoolSizes(sizes);

    vk::DescriptorPool pool = VulkanContext::Get().device.createDescriptorPool(createInfo);

    return GDescriptorPools.Add(std::move(pool));
}

DescriptorSetLayoutHandle Zn::RHIDevice::CreateDescriptorSetLayout(const RHI::DescriptorSetLayoutDescription& description_)
{
    Vector<vk::DescriptorSetLayoutBinding> bindings;

    for (const RHI::DescriptorSetLayoutBinding& binding : description_.bindings)
    {
        bindings.push_back(vk::DescriptorSetLayoutBinding {
            .binding         = binding.binding,
            .descriptorType  = RHI::TranslateDescriptorType(binding.descriptorType),
            .descriptorCount = binding.descriptorCount,
            .stageFlags      = TranslateShaderStagesMask(binding.shaderStages),
        });
    }

    vk::DescriptorSetLayout layout = VulkanContext::Get().device.createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo {
        .bindingCount = static_cast<uint32>(bindings.size()),
        .pBindings    = bindings.data(),
    });

    return GDescriptorSetLayouts.Add(std::move(layout));
}

ShaderModuleHandle Zn::RHIDevice::CreateShaderModule(Span<const uint8> shaderBytes_)
{
    vk::ShaderModule shader = VulkanContext::Get().device.createShaderModule(vk::ShaderModuleCreateInfo {
        .codeSize = shaderBytes_.Size(),
        .pCode    = (const uint32*) shaderBytes_.Data(),
    });

    return GShaders.Add(std::move(shader));
}

PipelineHandle Zn::RHIDevice::CreatePipeline(const RHI::PipelineDescription& description_)
{
    Vector<vk::DescriptorSetLayout> layouts {};
    for (const DescriptorSetLayoutHandle& handle : description_.descriptorSetLayouts)
    {
        vk::DescriptorSetLayout* layout = GDescriptorSetLayouts[handle];
        if (!layout)
        {
            ZN_LOG(LogVulkan, ELogVerbosity::Error, "Failed to create pipeline. Invalid Descriptor Set Layouts handle.");
            return PipelineHandle {};
        }

        layouts.push_back(*layout);
    }

    Vector<vk::PushConstantRange> pushConstants {};
    for (const RHI::PushConstantRange& range : description_.pushConstantRanges)
    {
        pushConstants.push_back(vk::PushConstantRange {
            .stageFlags = TranslateShaderStagesMask(range.stageFlags),
            .offset     = range.offset,
            .size       = range.size,
        });
    }

    Vector<vk::PipelineShaderStageCreateInfo> stages {};
    for (const RHI::Shader& shader : description_.shaders)
    {
        vk::ShaderModule* shaderModule = GShaders[shader.module];

        if (!shaderModule)
        {
            ZN_LOG(LogVulkan, ELogVerbosity::Error, "Failed to create shader stages. Invalid shader module handle.");
            return PipelineHandle {};
        }

        stages.push_back(vk::PipelineShaderStageCreateInfo {
            .stage  = TranslateShaderStage(shader.stage),
            .module = *shaderModule,
            .pName  = shader.entryPoint,
        });
    }

    vk::PipelineInputAssemblyStateCreateInfo inputAssembly {
        .topology               = vk::PrimitiveTopology::eTriangleList,
        .primitiveRestartEnable = false,
    };

    Vector<vk::VertexInputBindingDescription> vertexBindings;
    for (const RHI::VertexInputBinding& vertexInput : description_.vertexInputLayout.vertexInputs)
    {
        vertexBindings.push_back(vk::VertexInputBindingDescription {
            .binding   = vertexInput.binding,
            .stride    = vertexInput.stride,
            .inputRate = (vk::VertexInputRate) vertexInput.inputRate,
        });
    }
    Vector<vk::VertexInputAttributeDescription> vertexAttributes;
    for (const RHI::VertexInputAttribute& vertexAttribute : description_.vertexInputLayout.vertexAttributes)
    {
        vertexAttributes.push_back(vk::VertexInputAttributeDescription {
            .location = vertexAttribute.location,
            .binding  = vertexAttribute.binding,
            .format   = TranslateRHIFormat(vertexAttribute.format),
            .offset   = vertexAttribute.offset,
        });
    }

    vk::PipelineVertexInputStateCreateInfo vertexInput {
        .vertexBindingDescriptionCount   = (uint32) vertexBindings.size(),
        .pVertexBindingDescriptions      = vertexBindings.data(),
        .vertexAttributeDescriptionCount = (uint32) vertexAttributes.size(),
        .pVertexAttributeDescriptions    = vertexAttributes.data(),
    };

    vk::Viewport viewport {
        .x        = 0,
        .y        = 0,
        .width    = (float) (GSwapChain.extent.width),
        .height   = (float) (GSwapChain.extent.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };

    vk::Rect2D scissors {
        .offset =
            {
                .x = 0,
                .y = 0,
            },
        .extent = GSwapChain.extent,
    };

    vk::PipelineViewportStateCreateInfo viewportState {
        .viewportCount = 1,
        .pViewports    = &viewport,
        .scissorCount  = 1,
        .pScissors     = &scissors,
    };

    vk::PipelineRasterizationStateCreateInfo rasterization {
        .depthClampEnable        = false,
        .rasterizerDiscardEnable = false,
        .polygonMode             = TranslatePolygonMode(description_.polygonMode),
        .cullMode                = TranslateCullMode(description_.cullMode),
        .frontFace               = vk::FrontFace::eCounterClockwise,
        .depthBiasEnable         = false,
        .depthBiasConstantFactor = 0.0f,
        .depthBiasClamp          = 0.0f,
        .depthBiasSlopeFactor    = 0.0f,
        .lineWidth               = 1.0f,
    };

    vk::PipelineMultisampleStateCreateInfo msaa {
        .rasterizationSamples = vk::SampleCountFlagBits::e1,
        .sampleShadingEnable  = false,
        .minSampleShading     = 1.f,
    };

    Vector<vk::PipelineColorBlendAttachmentState> blendAttachments;
    for (const RHI::BlendAttachment& blendAttachment : description_.colorBlendState.attachments)
    {
        blendAttachments.push_back(vk::PipelineColorBlendAttachmentState {
            .blendEnable         = (vk::Bool32)(blendAttachment.enable),
            .srcColorBlendFactor = TranslateBlendFactor(blendAttachment.srcColorBlendFactor),
            .dstColorBlendFactor = TranslateBlendFactor(blendAttachment.dstColorBlendFactor),
            .colorBlendOp        = TranslateBlendOp(blendAttachment.colorBlendOp),
            .srcAlphaBlendFactor = TranslateBlendFactor(blendAttachment.srcAlphaBlendFactor),
            .dstAlphaBlendFactor = TranslateBlendFactor(blendAttachment.dstAlphaBlendFactor),
            .alphaBlendOp        = TranslateBlendOp(blendAttachment.alphaBlendOp),
            .colorWriteMask      = TranslateColorComponents(blendAttachment.colorWriteMask),
        });
    }

    vk::PipelineColorBlendStateCreateInfo colorBlending {
        .logicOpEnable   = description_.colorBlendState.enableLogicOp,
        .logicOp         = TranslateLogicOp(description_.colorBlendState.logicOp),
        .attachmentCount = (uint32) (blendAttachments.size()),
        .pAttachments    = blendAttachments.data(),
    };

    vk::PipelineDepthStencilStateCreateInfo depthStencil {
        .depthTestEnable  = description_.depthStencilState.enableDepthTest,
        .depthWriteEnable = description_.depthStencilState.enableDepthWrite,
        .depthCompareOp = description_.depthStencilState.enableDepthTest ? TranslateCompareOp(description_.depthStencilState.depthCompareOp)
                                                                         : vk::CompareOp::eAlways,
        .depthBoundsTestEnable = description_.depthStencilState.enableDepthBoundTest,
        .stencilTestEnable     = false, // description_.depthStencilState.enableStencilTest,
        .minDepthBounds        = description_.depthStencilState.minDepthBounds,
        .maxDepthBounds        = description_.depthStencilState.maxDepthBounds,
    };

    Span<const vk::DynamicState> dynamicStates({vk::DynamicState::eViewport, vk::DynamicState::eScissor});

    vk::PipelineDynamicStateCreateInfo dynamicState {
        .dynamicStateCount = (uint32) dynamicStates.Size(),
        .pDynamicStates    = dynamicStates.Data(),
    };

    VulkanRenderPass* renderPass = GRenderPasses[description_.renderPass];
    if (!renderPass)
    {
        ZN_LOG(LogVulkan, ELogVerbosity::Error, "Failed to create Graphics Pipeline. Render Pass is invalid");
        return PipelineHandle {};
    }

    vk::PipelineLayout pipelineLayout = VulkanContext::Get().device.createPipelineLayout(vk::PipelineLayoutCreateInfo {
        .setLayoutCount         = (uint32) layouts.size(),
        .pSetLayouts            = layouts.data(),
        .pushConstantRangeCount = (uint32) pushConstants.size(),
        .pPushConstantRanges    = pushConstants.data(),
    });

    check(pipelineLayout);

    vk::GraphicsPipelineCreateInfo pipelineInfo {
        .stageCount          = (uint32) stages.size(),
        .pStages             = stages.data(),
        .pVertexInputState   = &vertexInput,
        .pInputAssemblyState = &inputAssembly,
        .pViewportState      = &viewportState,
        .pRasterizationState = &rasterization,
        .pMultisampleState   = &msaa,
        .pDepthStencilState  = &depthStencil,
        .pColorBlendState    = &colorBlending,
        .pDynamicState       = &dynamicState,
        .layout              = pipelineLayout,
        .renderPass          = renderPass->renderPass,
        .subpass             = 0,
    };

    vk::ResultValue<vk::Pipeline> result = VulkanContext::Get().device.createGraphicsPipeline({}, pipelineInfo);

    if (result.result == vk::Result::eSuccess)
    {
        return GPipelines.Add({pipelineLayout, result.value});
    }
    else
    {
        VulkanContext::Get().device.destroyPipelineLayout(pipelineLayout);
        ZN_LOG(LogVulkan, ELogVerbosity::Error, "Failed to create Graphics Pipeline. Error: %d", result.result);
        return PipelineHandle {};
    }
}

void Zn::RHIDevice::CreateSwapChain()
{
    GSwapChain.Create();

    TTextureResource depthTexture = CreateRHITexture(RHITextureDescriptor {
        .width       = GSwapChain.extent.width,
        .height      = GSwapChain.extent.height,
        .numMips     = 1,
        .numChannels = 1,
        .channelSize = sizeof(float),
        .samples     = SampleCount::s1,
        .format      = RHIFormat::D32_Float,
        .flags       = RHITextureFlags::DepthStencil,
        .id          = GDepthTextureName,
        .debugName   = GDepthTextureName,
    });

    vk::ImageViewCreateInfo depthTextureViewCreateInfo {
        .image    = depthTexture.payload.image,
        .viewType = vk::ImageViewType::e2D,
        .format   = TranslateRHIFormat(RHIFormat::D32_Float),
        .subresourceRange =
            {
                .aspectMask     = vk::ImageAspectFlagBits::eDepth,
                .baseMipLevel   = 0,
                .levelCount     = 1,
                .baseArrayLayer = 0,
                .layerCount     = 1,
            },
    };

    depthTexture.payload.imageView = VulkanContext::Get().device.createImageView(depthTextureViewCreateInfo);

    GDepthTextureHandle = GTextures.Add(std::move(depthTexture));
}

void RHIDevice::CleanupSwapChain()
{
    VulkanContext& vkContext = VulkanContext::Get();

    for (VulkanRenderPass& renderPass : GRenderPasses)
    {
        for (vk::Framebuffer& frameBuffer : Span(renderPass.frameBuffer))
        {
            vkContext.device.destroyFramebuffer(frameBuffer);
            frameBuffer = VK_NULL_HANDLE;
        }
    }

    if (TTextureResource* depthTexture = GTextures[GDepthTextureHandle])
    {
        vkContext.device.destroyImageView(depthTexture->payload.imageView);
        vkContext.allocator.destroyImage(depthTexture->payload.image, depthTexture->payload.memory);

        GTextures.Evict(GDepthTextureHandle);

        GDepthTextureHandle = TextureHandle();
    }

    GSwapChain.Destroy();
}
