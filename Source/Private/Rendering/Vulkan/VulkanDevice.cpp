#include <Znpch.h>
#include <Core/Containers/Set.h>
#include <Core/IO/IO.h>
#include <Core/Memory/Memory.h>
#include <Core/CommandLine.h>
#include <vk_mem_alloc.h>
#include <Engine/Importer/MeshImporter.h>
#include <Engine/Importer/TextureImporter.h>
#include <Rendering/Material.h>
#include <Rendering/RHI/RHI.h>
#include <Rendering/RHI/RHIInputLayout.h>
#include <Rendering/RHI/RHIInstanceData.h>
#include <Rendering/RHI/RHIMesh.h>
#include <Rendering/RHI/RHITexture.h>
#include <Rendering/RHI/RHITypes.h>
#include <Rendering/Vulkan/VulkanDevice.h>
#include <Rendering/Vulkan/VulkanMaterialManager.h>
#include <Rendering/Vulkan/VulkanPipeline.h>
#include <SDL.h>
#include <algorithm>
#include <glm/gtx/matrix_decompose.hpp>

// ImGui

#include <imgui_impl_sdl.h>
#include <imgui_impl_vulkan.h>

using namespace Zn;

namespace
{
static const String         defaultTexturePath    = "assets/texture.jpg";
static const String         vikingRoomTexturePath = "assets/VulkanTutorial/viking_room.png";
static const String         vikingRoomMeshPath    = "assets/VulkanTutorial/viking_room.obj";
static const String         gltfSampleModels      = "assets/glTF-Sample-Models/2.0/";
static const String         gltfDefaultModel      = "assets/glTF-Sample-Models/2.0/Cube/glTF/Cube.gltf";
static const String         monkeyMeshPath        = "assets/VulkanGuide/monkey_smooth.obj";
static const ResourceHandle depthTextureHandle    = ResourceHandle(HashCalculate("__RHIDepthTexture"));
static const String         triangleMeshName      = "__Triangle";
static const String         whiteTextureName      = "__WhiteTexture";
static const String         blackTextureName      = "__BlackTexture";
static const String         normalTextureName     = "__Normal";
static const String         uvCheckerTexturePath  = "assets/uv_checker.png";
static const String         uvCheckerName         = "__UvChecker";

static const u32 minInstanceDataBufferSize = 65535;

const TextureSampler defaultTextureSampler {
    .minification  = SamplerFilter::Linear,
    .magnification = SamplerFilter::Linear,
    .mipMap        = SamplerFilter::None,
    .wrapUV        = {SamplerWrap::Repeat, SamplerWrap::Repeat, SamplerWrap::Repeat},
};

struct IndirectDrawBatch
{
    Material* material;
    RHIMesh*  mesh;
    u32       index;
    u32       count;
};

struct alignas(16) RHI_DrawData
{
    u32 materialIndex   = 0;
    u32 transformOffset = 0;
    u32 vertexOffset    = 0;
    u32 unused0         = 0; // Padding
};

String GetGLTFSampleModel()
{
    String out;
    CommandLine::Get().Value("-gltfModel", out, "");
    return out;
}

vk::Filter TranslateSamplerFilter(SamplerFilter filter)
{
    switch (filter)
    {
    case SamplerFilter::Nearest:
        return vk::Filter::eNearest;
    case SamplerFilter::Linear:
        return vk::Filter::eLinear;
    case SamplerFilter::None:
    case SamplerFilter::COUNT:
    default:
        return vk::Filter::eNearest;
    }
}

vk::SamplerAddressMode TranslateSamplerWrap(SamplerWrap wrap)
{
    switch (wrap)
    {
    case SamplerWrap::Repeat:
        return vk::SamplerAddressMode::eRepeat;
    case SamplerWrap::Clamp:
        return vk::SamplerAddressMode::eClampToEdge;
    case SamplerWrap::Mirrored:
        return vk::SamplerAddressMode::eMirroredRepeat;
    case SamplerWrap::COUNT:
    default:
        return vk::SamplerAddressMode::eRepeat;
    }
}
} // namespace

static const Zn::Vector<const char*> kRequiredExtensions = {VK_EXT_DEBUG_UTILS_EXTENSION_NAME};
static const Zn::Vector<const char*> kValidationLayers   = {"VK_LAYER_KHRONOS_validation"};
// static const Zn::Vector<const char*> kDeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

namespace VulkanValidation
{
#if ZN_VK_VALIDATION_LAYERS
bool SupportsValidationLayers()
{
    Zn::Vector<vk::LayerProperties> availableLayers = vk::enumerateInstanceLayerProperties();

    return std::any_of(availableLayers.begin(),
                       availableLayers.end(),
                       [](const vk::LayerProperties& it)
                       {
                           for (const auto& layerName : kValidationLayers)
                           {
                               if (strcmp(it.layerName, layerName) == 0)
                               {
                                   return true;
                               }
                           }

                           return false;
                       });
}

ELogVerbosity VkMessageSeverityToZnVerbosity(vk::DebugUtilsMessageSeverityFlagBitsEXT severity)
{
    switch (severity)
    {
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose:
        return ELogVerbosity::Verbose;
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo:
        return ELogVerbosity::Log;
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning:
        return ELogVerbosity::Warning;
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError:
    default:
        return ELogVerbosity::Error;
    }
}

VKAPI_ATTR VkBool32 VKAPI_CALL OnDebugMessage(VkDebugUtilsMessageSeverityFlagBitsEXT      severity,
                                              VkDebugUtilsMessageTypeFlagsEXT             type,
                                              const VkDebugUtilsMessengerCallbackDataEXT* data,
                                              void*                                       userData)
{
    ELogVerbosity verbosity = VkMessageSeverityToZnVerbosity(vk::DebugUtilsMessageSeverityFlagBitsEXT(severity));

    const String& messageType = vk::to_string(vk::DebugUtilsMessageTypeFlagsEXT(type));

    ZN_LOG(LogVulkanValidation, verbosity, "[%s] %s", messageType.c_str(), data->pMessage);

    if (verbosity >= ELogVerbosity::Error)
    {
        __debugbreak();
    }

    return VK_FALSE;
}

vk::DebugUtilsMessengerCreateInfoEXT GetDebugMessengerCreateInfo()
{
    vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo
    {
    #if ZN_VK_VALIDATION_VERBOSE
        .messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo | vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
                           vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
    #else
        .messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                           vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
    #endif
        .messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                       vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
        .pfnUserCallback = VulkanValidation::OnDebugMessage
    };

    return debugCreateInfo;
}

vk::DebugUtilsMessengerEXT GDebugMessenger {};

void InitializeDebugMessenger(vk::Instance instance)
{
    GDebugMessenger = instance.createDebugUtilsMessengerEXT(GetDebugMessengerCreateInfo());
}

void DeinitializeDebugMessenger(vk::Instance instance)
{
    if (GDebugMessenger)
    {
        instance.destroyDebugUtilsMessengerEXT(GDebugMessenger);
    }
}
#endif

template<typename VkHPPType>
void SetObjectDebugName(vk::Device device, cstring name, VkHPPType object)
{
#if ZN_VK_VALIDATION_LAYERS
    vk::DebugUtilsObjectNameInfoEXT nameInfo {
        .objectType   = VkHPPType::objectType,
        .objectHandle = (u64) static_cast<VkHPPType::CType>(object),
        .pObjectName  = name,
    };

    device.setDebugUtilsObjectNameEXT(nameInfo);
#endif
}
} // namespace VulkanValidation

const Vector<const char*> VulkanDevice::kDeviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

VulkanDevice::VulkanDevice()
{
}

VulkanDevice::~VulkanDevice()
{
    Cleanup();
}

void VulkanDevice::Initialize(SDL_Window* InWindowHandle)
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

    SDL_Window* window = InWindowHandle;

    u32 numExtensions = 0;
    SDL_Vulkan_GetInstanceExtensions(window, &numExtensions, nullptr);
    Vector<const char*> requiredExtensions(numExtensions);
    SDL_Vulkan_GetInstanceExtensions(window, &numExtensions, requiredExtensions.data());

    vk::InstanceCreateInfo instanceCreateInfo {.pApplicationInfo = &appInfo};

    Vector<const char*> enabledLayers;

#if ZN_VK_VALIDATION_LAYERS
    // Request debug utils extension if validation layers are enabled.
    numExtensions += static_cast<u32>(kRequiredExtensions.size());
    requiredExtensions.insert(requiredExtensions.end(), kRequiredExtensions.begin(), kRequiredExtensions.end());

    if (VulkanValidation::SupportsValidationLayers())
    {
        enabledLayers.insert(enabledLayers.begin(), kValidationLayers.begin(), kValidationLayers.end());

        VkDebugUtilsMessengerCreateInfoEXT debugMessagesInfo = VulkanValidation::GetDebugMessengerCreateInfo();
        instanceCreateInfo.pNext                             = &debugMessagesInfo;
    }
#endif

    instanceCreateInfo.enabledExtensionCount   = numExtensions;
    instanceCreateInfo.ppEnabledExtensionNames = requiredExtensions.data();
    instanceCreateInfo.enabledLayerCount       = (u32) enabledLayers.size();
    instanceCreateInfo.ppEnabledLayerNames     = enabledLayers.data();

    instance = vk::createInstance(instanceCreateInfo);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(instance);

#if ZN_VK_VALIDATION_LAYERS
    VulkanValidation::InitializeDebugMessenger(instance);
#endif

    VkSurfaceKHR sdlSurface;
    if (SDL_Vulkan_CreateSurface(window, instance, &sdlSurface) != SDL_TRUE)
    {
        _ASSERT(false);
    }

    surface = sdlSurface;

    windowID = SDL_GetWindowID(InWindowHandle);

    /////// Initialize GPU

    Vector<vk::PhysicalDevice> devices = instance.enumeratePhysicalDevices();

    gpu = SelectPhysicalDevice(devices);

    _ASSERT(gpu);

    gpuFeatures = gpu.getFeatures();

    /////// Initialize Logical Device

    QueueFamilyIndices Indices = GetQueueFamilyIndices(gpu);

    SwapChainDetails SwapChainDetails = GetSwapChainDetails(gpu);

    const bool IsSupported = SwapChainDetails.Formats.size() > 0 && SwapChainDetails.PresentModes.size() > 0;

    if (!IsSupported)
    {
        _ASSERT(false);
        return;
    }

    Vector<vk::DeviceQueueCreateInfo> queueFamilies = BuildQueueCreateInfo(Indices);

    vk::DeviceCreateInfo deviceCreateInfo {
        .queueCreateInfoCount    = static_cast<u32>(queueFamilies.size()),
        .pQueueCreateInfos       = queueFamilies.data(),
        .enabledExtensionCount   = static_cast<u32>(kDeviceExtensions.size()),
        .ppEnabledExtensionNames = kDeviceExtensions.data(),
        .pEnabledFeatures        = &gpuFeatures,
    };

    device = gpu.createDevice(deviceCreateInfo, nullptr);

    VULKAN_HPP_DEFAULT_DISPATCHER.init(device);

    graphicsQueue = device.getQueue(Indices.Graphics.value(), 0);
    presentQueue  = device.getQueue(Indices.Present.value(), 0);

    // initialize the memory allocator
    vma::AllocatorCreateInfo allocatorCreateInfo {.physicalDevice = gpu, .device = device, .instance = instance};

    allocator = vma::createAllocator(allocatorCreateInfo);

    CreateDescriptors();

    /////// Create Swap Chain

    CreateSwapChain();

    CreateImageViews();

    ////// Command Pool

    vk::CommandPoolCreateInfo graphicsPoolCreateInfo {// we also want the pool to allow for resetting of individual command buffers
                                                      .flags            = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
                                                      // the command pool will be one that can submit graphics commands
                                                      .queueFamilyIndex = Indices.Graphics.value()};

    commandPool = device.createCommandPool(graphicsPoolCreateInfo);

    destroyQueue.Enqueue(
        [=]()
        {
            device.destroyCommandPool(commandPool);
        });

    ////// Command Buffer

    vk::CommandBufferAllocateInfo graphicsCmdCreateInfo {
        .commandPool = commandPool, .level = vk::CommandBufferLevel::ePrimary, .commandBufferCount = kMaxFramesInFlight};

    commandBuffers = device.allocateCommandBuffers(graphicsCmdCreateInfo);

    for (i32 index = 0; index < commandBuffers.size(); ++index)
    {
        String contextName      = "commandBuffer" + std::to_string(index);
        gpuTraceContexts[index] = ZN_TRACE_GPU_CONTEXT_CREATE(contextName.c_str(), gpu, device, graphicsQueue, commandBuffers[index]);
    }

    // Upload Context

    vk::CommandPoolCreateInfo uploadCmdPoolCreateInfo {.flags            = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
                                                       .queueFamilyIndex = Indices.Graphics.value()};

    uploadContext.commandPool = device.createCommandPool(graphicsPoolCreateInfo);

    destroyQueue.Enqueue(
        [=]()
        {
            device.destroyCommandPool(uploadContext.commandPool);
        });

    vk::CommandBufferAllocateInfo uploadCmdBufferCreateInfo {
        .commandPool = uploadContext.commandPool, .level = vk::CommandBufferLevel::ePrimary, .commandBufferCount = 1};

    uploadContext.cmdBuffer = device.allocateCommandBuffers(uploadCmdBufferCreateInfo)[0];

    vk::FenceCreateInfo uploadFenceCreateInfo {};
    uploadContext.fence = device.createFence(uploadFenceCreateInfo);

    destroyQueue.Enqueue(
        [=]()
        {
            device.destroyFence(uploadContext.fence);
        });

    uploadContext.traceContext = ZN_TRACE_GPU_CONTEXT_CREATE("upload", gpu, device, graphicsQueue, uploadContext.cmdBuffer);

    destroyQueue.Enqueue(
        [=]()
        {
            ZN_TRACE_GPU_CONTEXT_DESTROY(uploadContext.traceContext);
        });

    ////// Render Pass

    //	Color Attachment

    // the renderpass will use this color attachment.
    vk::AttachmentDescription colorAttachmentDesc {
        .format         = swapChainFormat.format,
        // 1 sample, we won't be doing MSAA
        .samples        = vk::SampleCountFlagBits::e1,
        // we Clear when this attachment is loaded
        .loadOp         = vk::AttachmentLoadOp::eClear,
        // we keep the attachment stored when the renderpass ends
        .storeOp        = vk::AttachmentStoreOp::eStore,
        // we don't care about stencil
        .stencilLoadOp  = vk::AttachmentLoadOp::eDontCare,
        .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
        // we don't know or care about the starting layout of the attachment
        .initialLayout  = vk::ImageLayout::eUndefined,
        // after the renderpass ends, the image has to be on a layout ready for display
        .finalLayout    = vk::ImageLayout::ePresentSrcKHR,
    };

    vk::AttachmentReference colorAttachmentRef {
        .attachment = 0,
        .layout     = vk::ImageLayout::eColorAttachmentOptimal,
    };

    //	Depth Attachment
    //	Both the depth attachment and its reference are copypaste of the color one, as it works the same, but
    // with a small change: 	.format = m_DepthFormat; is set to the depth format that we created the depth image
    // at. 	.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    vk::AttachmentDescription depthAttachmentDesc {
        .flags          = vk::AttachmentDescriptionFlags(0),
        // TODO
        .format         = textures[depthTextureHandle]->format,
        .samples        = vk::SampleCountFlagBits::e1,
        .loadOp         = vk::AttachmentLoadOp::eClear,
        .storeOp        = vk::AttachmentStoreOp::eStore,
        .stencilLoadOp  = vk::AttachmentLoadOp::eClear,
        .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
        .initialLayout  = vk::ImageLayout::eUndefined,
        .finalLayout    = vk::ImageLayout::eDepthStencilAttachmentOptimal,
    };

    vk::AttachmentReference depthAttachmentRef {.attachment = 1, .layout = vk::ImageLayout::eDepthStencilAttachmentOptimal};

    // Main Subpass

    // we are going to create 1 subpass, which is the minimum you can do
    vk::SubpassDescription subpassDesc {
        .pipelineBindPoint       = vk::PipelineBindPoint::eGraphics,
        .colorAttachmentCount    = 1,
        .pColorAttachments       = &colorAttachmentRef,
        .pDepthStencilAttachment = &depthAttachmentRef,
    };

    /*
     * https://vulkan-tutorial.com/Drawing_a_triangle/Drawing/Rendering_and_presentation | Subpass dependencies
     */
    vk::SubpassDependency colorDependency {.srcSubpass    = VK_SUBPASS_EXTERNAL,
                                           .dstSubpass    = 0,
                                           .srcStageMask  = vk::PipelineStageFlagBits::eColorAttachmentOutput,
                                           .dstStageMask  = vk::PipelineStageFlagBits::eColorAttachmentOutput,
                                           .srcAccessMask = vk::AccessFlags(0),
                                           .dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite};

    //	Add a new dependency that synchronizes access to depth attachments.
    //	Without this multiple frames can be rendered simultaneously by the GPU.

    vk::SubpassDependency depthDependency {
        .srcSubpass    = VK_SUBPASS_EXTERNAL,
        .dstSubpass    = 0,
        .srcStageMask  = vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests,
        .dstStageMask  = vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests,
        .srcAccessMask = vk::AccessFlags(0),
        .dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite};

    vk::AttachmentDescription attachments[2]  = {colorAttachmentDesc, depthAttachmentDesc};
    vk::SubpassDependency     dependencies[2] = {colorDependency, depthDependency};
    vk::RenderPassCreateInfo  renderPassCreateInfo {.subpassCount = 1, .pSubpasses = &subpassDesc};

    renderPassCreateInfo.setAttachments(attachments);
    renderPassCreateInfo.setDependencies(dependencies);

    renderPass = device.createRenderPass(renderPassCreateInfo);

    destroyQueue.Enqueue(
        [=]()
        {
            device.destroyRenderPass(renderPass);
        });

    /////// Frame Buffers

    CreateFramebuffers();

    ////// Sync Structures

    // we want to create the fence with the Create Signaled flag, so we can wait on it before using it on a GPU command (for the first
    // frame)
    vk::FenceCreateInfo fenceCreateInfo {.flags = vk::FenceCreateFlagBits::eSignaled};

    for (size_t index = 0; index < kMaxFramesInFlight; ++index)
    {
        renderFences[index] = device.createFence(fenceCreateInfo);
        destroyQueue.Enqueue(
            [=]()
            {
                device.destroyFence(renderFences[index]);
            });
    }

    // for the semaphores we don't need any flags
    vk::SemaphoreCreateInfo semaphoreCreateInfo {};

    for (size_t Index = 0; Index < kMaxFramesInFlight; ++Index)
    {
        presentSemaphores[Index] = device.createSemaphore(semaphoreCreateInfo);
        renderSemaphores[Index]  = device.createSemaphore(semaphoreCreateInfo);

        destroyQueue.Enqueue(
            [=]()
            {
                device.destroySemaphore(presentSemaphores[Index]);
                device.destroySemaphore(renderSemaphores[Index]);
            });
    }

    ////// ImGui
    {
        ImGui_ImplSDL2_InitForVulkan(InWindowHandle);

        // 1: create descriptor pool for IMGUI
        // the size of the pool is very oversize, but it's copied from imgui demo itself.

        Vector<vk::DescriptorPoolSize> imguiPoolSizes = {{vk::DescriptorType::eSampler, 1000},
                                                         {vk::DescriptorType::eCombinedImageSampler, 1000},
                                                         {vk::DescriptorType::eSampledImage, 1000},
                                                         {vk::DescriptorType::eStorageImage, 1000},
                                                         {vk::DescriptorType::eUniformTexelBuffer, 1000},
                                                         {vk::DescriptorType::eStorageTexelBuffer, 1000},
                                                         {vk::DescriptorType::eUniformBuffer, 1000},
                                                         {vk::DescriptorType::eStorageBuffer, 1000},
                                                         {vk::DescriptorType::eUniformBufferDynamic, 1000},
                                                         {vk::DescriptorType::eStorageBufferDynamic, 1000},
                                                         {vk::DescriptorType::eInputAttachment, 1000}};

        vk::DescriptorPoolCreateInfo imguiPoolCreateInfo {
            .flags   = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
            .maxSets = 1000,
        };

        imguiPoolCreateInfo.setPoolSizes(imguiPoolSizes);

        imguiDescriptorPool = device.createDescriptorPool(imguiPoolCreateInfo);

        destroyQueue.Enqueue(
            [=]()
            {
                device.destroyDescriptorPool(imguiDescriptorPool);
            });

        // this initializes imgui for Vulkan
        ImGui_ImplVulkan_InitInfo imguiInitInfo {.Instance       = instance,
                                                 .PhysicalDevice = gpu,
                                                 .Device         = device,
                                                 .Queue          = graphicsQueue,
                                                 .DescriptorPool = imguiDescriptorPool,
                                                 .MinImageCount  = 3,
                                                 .ImageCount     = 3,
                                                 .MSAASamples    = VK_SAMPLE_COUNT_1_BIT};

        ImGui_ImplVulkan_Init(&imguiInitInfo, renderPass);

        // Upload Fonts

        vk::CommandBuffer commandBuffer = commandBuffers[0];
        commandBuffer.reset();

        commandBuffer.begin(vk::CommandBufferBeginInfo {.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});

        ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);

        commandBuffer.end();

        vk::CommandBuffer commandBuffers[1] = {commandBuffer};
        graphicsQueue.submit(vk::SubmitInfo {.commandBufferCount = 1, .pCommandBuffers = &commandBuffers[0]});

        device.waitIdle();

        ImGui_ImplVulkan_DestroyFontUploadObjects();
    }

    CreateDefaultResources();
    LoadMeshes();

    {
        CreateTexture(defaultTexturePath);
        CreateTexture(vikingRoomTexturePath);
    }

    CreateMeshPipeline();

    if (gpuFeatures.multiDrawIndirect)
    {
        for (i32 index = 0; index < kMaxFramesInFlight; ++index)
        {
            drawCommands[index] =
                CreateBuffer(sizeof(vk::DrawIndexedIndirectCommand) * 4096,
                             vk::BufferUsageFlagBits::eIndirectBuffer,
                             vma::MemoryUsage::eAutoPreferHost,
                             vma::AllocationCreateFlagBits::eHostAccessSequentialWrite | vma::AllocationCreateFlagBits::eMapped);

            destroyQueue.Enqueue(
                [=]()
                {
                    DestroyBuffer(drawCommands[index]);
                });
        }
    }

    CreateScene();
}

void VulkanDevice::Cleanup()
{
    if (!instance)
    {
        return;
    }

    device.waitIdle();

    CleanupSwapChain();

    for (auto& textureKvp : textures)
    {
        device.destroyImageView(textureKvp.second->imageView);
        allocator.destroyImage(textureKvp.second->image, textureKvp.second->allocation);

        if (textureKvp.second->sampler)
        {
            device.destroySampler(textureKvp.second->sampler);
        }

        delete textureKvp.second;
    }

    for (RHIPrimitiveGPU* gpuPrimitive : gpuPrimitives)
    {
        DestroyBuffer(gpuPrimitive->position);
        DestroyBuffer(gpuPrimitive->normal);
        DestroyBuffer(gpuPrimitive->tangent);
        DestroyBuffer(gpuPrimitive->uv);
        DestroyBuffer(gpuPrimitive->color);
        DestroyBuffer(gpuPrimitive->indices);
        DestroyBuffer(gpuPrimitive->uboMaterialAttributes);

        delete gpuPrimitive;
    }

    gpuPrimitives.clear();

    for (auto& meshKvp : meshes)
    {
        allocator.destroyBuffer(meshKvp.second->vertexBuffer.data, meshKvp.second->vertexBuffer.allocation);

        if (meshKvp.second->indexBuffer.data)
        {
            allocator.destroyBuffer(meshKvp.second->indexBuffer.data, meshKvp.second->indexBuffer.allocation);
        }
    }

    textures.clear();

    meshes.clear();

    for (i32 index = 0; index < kMaxFramesInFlight; ++index)
    {
        ZN_TRACE_GPU_CONTEXT_DESTROY(gpuTraceContexts[index]);
    }

    destroyQueue.Flush();

    ImGui_ImplVulkan_Shutdown();

    for (size_t Index = 0; Index < kMaxFramesInFlight; ++Index)
    {
        commandBuffers[Index] = VK_NULL_HANDLE;
    }

    uploadContext.cmdBuffer = VK_NULL_HANDLE;

    frameBuffers.clear();
    swapChainImageViews.clear();

    allocator.destroy();

    if (surface)
    {
        instance.destroySurfaceKHR(surface);
    }

#if ZN_VK_VALIDATION_LAYERS
    VulkanValidation::DeinitializeDebugMessenger(instance);
#endif

    if (device)
    {
        device.destroy();
    }

    instance.destroy();
}

void Zn::VulkanDevice::BeginFrame()
{
    // wait until the GPU has finished rendering the last frame. Timeout of 1 second

    ZN_VK_CHECK(device.waitForFences({renderFences[currentFrame]}, true, kWaitTimeOneSecond));

    if (isMinimized)
    {
        return;
    }

    static constexpr auto kSwapChainWaitTime = 1000000000;
    // request image from the swapchain, one second timeout
    // m_VkPresentSemaphore is set to make sure that we can sync other operations with the swapchain having an image ready to render.
    vk::Result            acquireImageResult =
        device.acquireNextImageKHR(swapChain, kSwapChainWaitTime, presentSemaphores[currentFrame], nullptr /*fence*/, &swapChainImageIndex);

    if (acquireImageResult == vk::Result::eErrorOutOfDateKHR)
    {
        RecreateSwapChain();
        return;
    }
    else if (acquireImageResult != vk::Result::eSuccess && acquireImageResult != vk::Result::eSuboptimalKHR)
    {
        _ASSERT(false);
        return;
    }

    device.resetFences({renderFences[currentFrame]});

    // now that we are sure that the commands finished executing, we can safely reset the command buffer to begin recording again.
    commandBuffers[currentFrame].reset();
}

void VulkanDevice::Draw()
{
    ZN_TRACE_QUICKSCOPE();

    if (isMinimized)
    {
        return;
    }

    // Build ImGui render commands
    ImGui::Render();

    // naming it commandBuffer for shorter writing
    vk::CommandBuffer commandBuffer = commandBuffers[currentFrame];

    // begin the command buffer recording. We will use this command buffer exactly once, so we want to let Vulkan know that

    commandBuffer.begin(vk::CommandBufferBeginInfo {.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});

    {
        ZN_TRACE_GPU_SCOPE("Draw", gpuTraceContexts[currentFrame], commandBuffer);

        vk::ClearValue clearColor(vk::ClearColorValue(1.0f, 1.0f, 1.0f, 1.0f));

        vk::ClearValue depthClear;

        //	make a clear-color from frame number. This will flash with a 120 frame period.
        depthClear.color              = vk::ClearColorValue {0.0f, 0.0f, abs(sin(frameNumber / 120.f)), 1.0f};
        depthClear.depthStencil.depth = 1.f;

        // start the main renderpass.
        // We will use the clear color from above, and the framebuffer of the index the swapchain gave us
        vk::RenderPassBeginInfo renderPassBeginInfo {};

        renderPassBeginInfo.renderPass          = renderPass;
        renderPassBeginInfo.renderArea.offset.x = 0;
        renderPassBeginInfo.renderArea.offset.y = 0;
        renderPassBeginInfo.renderArea.extent   = swapChainExtent;
        renderPassBeginInfo.framebuffer         = frameBuffers[swapChainImageIndex];

        //	Connect clear values

        vk::ClearValue clearValues[2] = {clearColor, depthClear};
        renderPassBeginInfo.setClearValues(clearValues);

        commandBuffer.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);

        // *** Insert Commands here ***
        {
            // Model View Matrix

            // Camera View
            const glm::mat4 view = glm::lookAt(cameraPosition, cameraPosition + cameraDirection, upVector);

            // Camera Projection
            glm::mat4 projection = glm::perspective(glm::radians(60.f), 16.f / 9.f, 0.05f, 20000.f);
            projection[1][1] *= -1;

            GPUCameraData camera {};
            camera.position        = glm::vec4(cameraPosition, 1.0f);
            camera.projection      = projection;
            camera.view            = view;
            camera.view_projection = projection * view;

            CopyToGPU(cameraBuffer[currentFrame].allocation, &camera, sizeof(GPUCameraData));

            LightingUniforms lighting {};
            lighting.directional_lights[0].direction = glm::normalize(glm::vec4(-1.0f, -3.0f, 0.0f, 1.0f));
            lighting.directional_lights[0].color     = glm::vec4(1.0f, 1.f, 1.f, 0.f);
            lighting.directional_lights[0].intensity = 0.25f;
            lighting.ambient_light.color             = glm::vec4(1.f, 1.f, 1.f, 0.f);
            lighting.ambient_light.intensity         = 0.0f;
            lighting.num_directional_lights          = 1;
            lighting.num_point_lights                = 0;

            CopyToGPU(lightingBuffer[currentFrame].allocation, &lighting, sizeof(LightingUniforms));

            // Vector<RHIInstanceData> meshData;
            // meshData.reserve(renderables.size());

            // for (const RenderObject& object : renderables)
            //{
            //     static const auto identity = glm::mat4 {1.f};

            //    glm::mat4 translation = glm::translate(identity, object.location);
            //    glm::mat4 rotation    = glm::mat4_cast(object.rotation);
            //    glm::mat4 scale       = glm::scale(identity, object.scale);

            //    glm::mat4 transform = translation * rotation * scale;

            //    meshData.push_back(RHIInstanceData {.m = transform});
            //}

            // CopyToGPU(instanceData[currentFrame].allocation, meshData.data(), meshData.size() * sizeof(RHIInstanceData));

            DrawObjects(commandBuffer, renderables.data(), renderables.size());

            // Enqueue ImGui commands to CmdBuffer
            ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
        }

        // finalize the render pass
        commandBuffer.endRenderPass();
        // finalize the command buffer (we can no longer add commands, but it can now be executed)
    }
    commandBuffer.end();
}

void Zn::VulkanDevice::EndFrame()
{
    ZN_TRACE_QUICKSCOPE();

    if (isMinimized)
    {
        return;
    }

    auto& CmdBuffer = commandBuffers[currentFrame];
    ////// Submit

    // prepare the submission to the queue.
    // we want to wait on the m_VkPresentSemaphore, as that semaphore is signaled when the swapchain is ready
    // we will signal the m_VkRenderSemaphore, to signal that rendering has finished

    vk::Semaphore waitSemaphores[]   = {presentSemaphores[currentFrame]};
    vk::Semaphore signalSemaphores[] = {renderSemaphores[currentFrame]};

    vk::SubmitInfo submitInfo {};

    vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;

    submitInfo.pWaitDstStageMask = &waitStage;

    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores    = waitSemaphores;

    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores    = signalSemaphores;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers    = &CmdBuffer;

    // submit command buffer to the queue and execute it.
    //  _renderFence will now block until the graphic commands finish execution
    graphicsQueue.submit(submitInfo, renderFences[currentFrame]);

    ////// Present

    // this will put the image we just rendered into the visible window.
    // we want to wait on the _renderSemaphore for that,
    // as it's necessary that drawing commands have finished before the image is displayed to the user
    vk::PresentInfoKHR presentInfo = {};

    presentInfo.pSwapchains    = &swapChain;
    presentInfo.swapchainCount = 1;

    presentInfo.pWaitSemaphores    = signalSemaphores;
    presentInfo.waitSemaphoreCount = 1;

    presentInfo.pImageIndices = &swapChainImageIndex;

    vk::Result presentResult = presentQueue.presentKHR(presentInfo);

    if (presentResult == vk::Result::eErrorOutOfDateKHR || presentResult == vk::Result::eSuboptimalKHR)
    {
        RecreateSwapChain();
    }
    else
    {
        _ASSERT(presentResult == vk::Result::eSuccess);
    }

    ZN_TRACE_GPU_COLLECT(gpuTraceContexts[currentFrame], commandBuffers[currentFrame]);

    currentFrame = (currentFrame + 1) % kMaxFramesInFlight;
    frameNumber++;
}

void Zn::VulkanDevice::ResizeWindow()
{
    RecreateSwapChain();
}

void Zn::VulkanDevice::OnWindowMinimized()
{
    isMinimized = true;
}

void Zn::VulkanDevice::OnWindowRestored()
{
    isMinimized = false;
}

bool VulkanDevice::HasRequiredDeviceExtensions(vk::PhysicalDevice inDevice) const
{
    Vector<vk::ExtensionProperties> availableExtensions = inDevice.enumerateDeviceExtensionProperties();

    static const Set<String> kRequiredDeviceExtensions(kDeviceExtensions.begin(), kDeviceExtensions.end());

    auto numFoundExtensions = std::count_if(availableExtensions.begin(),
                                            availableExtensions.end(),
                                            [](const vk::ExtensionProperties& extension)
                                            {
                                                return kRequiredDeviceExtensions.contains(extension.extensionName);
                                            });

    return kRequiredDeviceExtensions.size() == numFoundExtensions;
}

vk::PhysicalDevice VulkanDevice::SelectPhysicalDevice(const Vector<vk::PhysicalDevice>& inDevices) const
{
    i32 num = inDevices.size();

    i32 selectedIndex = std::numeric_limits<i32>::max();
    u32 maxScore      = 0;

    for (i32 idx = 0; idx < num; ++idx)
    {
        vk::PhysicalDevice device = inDevices[idx];

        u32 deviceScore = 0;

        const bool hasGraphicsQueue = GetQueueFamilyIndices(device).Graphics.has_value();

        const bool hasRequiredExtensions = HasRequiredDeviceExtensions(device);

        if (hasGraphicsQueue && hasRequiredExtensions)
        {
            vk::PhysicalDeviceProperties deviceProperties = device.getProperties();
            vk::PhysicalDeviceFeatures   deviceFeatures   = device.getFeatures();

            if (deviceProperties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
            {
                deviceScore += 1000;
            }

            // Texture size influences quality
            deviceScore += deviceProperties.limits.maxImageDimension2D;

            // vkCmdDrawIndirect support
            if (deviceFeatures.multiDrawIndirect)
            {
                deviceScore += 500;
            }

            // TODO: Add more criteria to choose GPU.
        }

        if (deviceScore > maxScore && deviceScore != 0)
        {
            maxScore      = deviceScore;
            selectedIndex = idx;
        }
    }

    if (selectedIndex != std::numeric_limits<size_t>::max())
    {
        return inDevices[selectedIndex];
    }
    else
    {
        return VK_NULL_HANDLE;
    }
}

QueueFamilyIndices VulkanDevice::GetQueueFamilyIndices(vk::PhysicalDevice inDevice) const
{
    QueueFamilyIndices outIndices;

    Vector<vk::QueueFamilyProperties> queueFamilies = inDevice.getQueueFamilyProperties();

    for (u32 idx = 0; idx < queueFamilies.size(); ++idx)
    {
        const vk::QueueFamilyProperties& queueFamily = queueFamilies[idx];

        if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
        {
            outIndices.Graphics = idx;
        }

        // tbd: We could enforce that Graphics and Present are in the same queue but is not mandatory.
        if (inDevice.getSurfaceSupportKHR(idx, surface) == VK_TRUE)
        {
            outIndices.Present = idx;
        }
    }

    return outIndices;
}

SwapChainDetails VulkanDevice::GetSwapChainDetails(vk::PhysicalDevice inGPU) const
{
    SwapChainDetails outDetails {.Capabilities = inGPU.getSurfaceCapabilitiesKHR(surface),
                                 .Formats      = inGPU.getSurfaceFormatsKHR(surface),
                                 .PresentModes = inGPU.getSurfacePresentModesKHR(surface)};

    return outDetails;
}

Vector<vk::DeviceQueueCreateInfo> VulkanDevice::BuildQueueCreateInfo(const QueueFamilyIndices& InIndices) const
{
    UnorderedSet<u32> queues = {InIndices.Graphics.value(), InIndices.Present.value()};

    Vector<vk::DeviceQueueCreateInfo> outCreateInfo {};
    outCreateInfo.reserve(queues.size());

    for (uint32 queueIdx : queues)
    {
        vk::DeviceQueueCreateInfo queueInfo;
        queueInfo.queueFamilyIndex = queueIdx;
        queueInfo.queueCount       = 1;

        static const float kQueuePriority = 1.0f;

        queueInfo.pQueuePriorities = &kQueuePriority;

        outCreateInfo.emplace_back(std::move(queueInfo));
    }

    return outCreateInfo;
}

vk::ShaderModule Zn::VulkanDevice::CreateShaderModule(const Vector<uint8>& bytes)
{
    vk::ShaderModuleCreateInfo shaderCreateInfo {};
    shaderCreateInfo.codeSize = bytes.size();
    shaderCreateInfo.pCode    = reinterpret_cast<const uint32_t*>(bytes.data());

    return device.createShaderModule(shaderCreateInfo);
}

void Zn::VulkanDevice::CreateDescriptors()
{
    // Create Descriptor Pool
    vk::DescriptorPoolSize poolSizes[] = {
        {vk::DescriptorType::eCombinedImageSampler, 1000},
        {vk::DescriptorType::eUniformBuffer, 1000},
    };

    vk::DescriptorPoolCreateInfo poolCreateInfo {.flags         = (vk::DescriptorPoolCreateFlagBits) 0,
                                                 .maxSets       = 1000,
                                                 .poolSizeCount = ArrayLength(poolSizes),
                                                 .pPoolSizes    = ArrayData(poolSizes)};

    descriptorPool = device.createDescriptorPool(poolCreateInfo);

    destroyQueue.Enqueue(
        [=]()
        {
            device.destroyDescriptorPool(descriptorPool);
        });

    // Global Set

    vk::DescriptorSetLayoutBinding bindings[] = {
        // Camera
        {
            .binding         = 0,
            .descriptorType  = vk::DescriptorType::eUniformBuffer,
            .descriptorCount = 1,
            .stageFlags      = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
        },
        // Lighting
        {
            .binding         = 1,
            .descriptorType  = vk::DescriptorType::eUniformBuffer,
            .descriptorCount = 1,
            .stageFlags      = vk::ShaderStageFlagBits::eFragment,
        },
        // Matrices
        /*{.binding         = 2,
         .descriptorType  = vk::DescriptorType::eUniformBufferDynamic,
         .descriptorCount = 1,
         .stageFlags      = vk::ShaderStageFlagBits::eVertex}*/
    };

    vk::DescriptorSetLayoutCreateInfo globalSetCreateInfo {.bindingCount = ArrayLength(bindings), .pBindings = ArrayData(bindings)};

    globalDescriptorSetLayout = device.createDescriptorSetLayout(globalSetCreateInfo);

    destroyQueue.Enqueue(
        [=]()
        {
            device.destroyDescriptorSetLayout(globalDescriptorSetLayout);
        });

    globalDescriptorSets.resize(kMaxFramesInFlight);

    for (size_t Index = 0; Index < kMaxFramesInFlight; ++Index)
    {
        // Camera

        cameraBuffer[Index] = CreateBuffer(sizeof(GPUCameraData), vk::BufferUsageFlagBits::eUniformBuffer, vma::MemoryUsage::eCpuToGpu);

        destroyQueue.Enqueue(
            [=]()
            {
                DestroyBuffer(cameraBuffer[Index]);
            });

        // Lighting

        lightingBuffer[Index] =
            CreateBuffer(sizeof(LightingUniforms), vk::BufferUsageFlagBits::eUniformBuffer, vma::MemoryUsage::eCpuToGpu);

        destroyQueue.Enqueue(
            [=]()
            {
                DestroyBuffer(lightingBuffer[Index]);
            });

        // Matrices

        /* if (gpuFeatures.multiDrawIndirect)
         {
             instanceData[Index] = CreateBuffer(
                 sizeof(RHIInstanceData) * minInstanceDataBufferSize, vk::BufferUsageFlagBits::eVertexBuffer, vma::MemoryUsage::eCpuToGpu);
         }
         else
         {
             instanceData[Index] = CreateBuffer(
                 sizeof(RHIInstanceData) * minInstanceDataBufferSize, vk::BufferUsageFlagBits::eUniformBuffer, vma::MemoryUsage::eCpuToGpu);
         }

         destroyQueue.Enqueue(
             [=]()
             {
                 DestroyBuffer(instanceData[Index]);
             });*/

        vk::DescriptorSetAllocateInfo descriptorSetAllocInfo {};
        descriptorSetAllocInfo.descriptorPool     = descriptorPool;
        descriptorSetAllocInfo.descriptorSetCount = 1;
        descriptorSetAllocInfo.pSetLayouts        = &globalDescriptorSetLayout;

        globalDescriptorSets[Index] = device.allocateDescriptorSets(descriptorSetAllocInfo)[0];

        vk::DescriptorBufferInfo bufferInfo[] = {vk::DescriptorBufferInfo {cameraBuffer[Index].data, 0, sizeof(GPUCameraData)},
                                                 vk::DescriptorBufferInfo {lightingBuffer[Index].data, 0, sizeof(LightingUniforms)},
                                                 /*            vk::DescriptorBufferInfo {instanceData[Index].data, 0, sizeof(MeshData)}*/};

        Vector<vk::WriteDescriptorSet> descriptorSetWrites = {
            vk::WriteDescriptorSet {
                .dstSet          = globalDescriptorSets[Index],
                .dstBinding      = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType  = vk::DescriptorType::eUniformBuffer,
                .pBufferInfo     = &bufferInfo[0],
            },
            vk::WriteDescriptorSet {
                .dstSet          = globalDescriptorSets[Index],
                .dstBinding      = 1,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType  = vk::DescriptorType::eUniformBuffer,
                .pBufferInfo     = &bufferInfo[1],
            },
/*            vk::WriteDescriptorSet {
                .dstSet          = globalDescriptorSets[Index],
                .dstBinding      = 2,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType  = vk::DescriptorType::eUniformBufferDynamic,
                .pBufferInfo     = &bufferInfo[2],
            }*/};

        device.updateDescriptorSets(descriptorSetWrites, {});
    }
}

void Zn::VulkanDevice::CreateSwapChain()
{
    SwapChainDetails SwapChainDetails = GetSwapChainDetails(gpu);

    QueueFamilyIndices Indices = GetQueueFamilyIndices(gpu);

    vk::SurfaceFormatKHR surfaceFormat = {vk::Format::eUndefined, vk::ColorSpaceKHR::eSrgbNonlinear};

    for (const vk::SurfaceFormatKHR& availableFormat : SwapChainDetails.Formats)
    {
        if (availableFormat.format == vk::Format::eB8G8R8A8Srgb && availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
        {
            surfaceFormat = availableFormat;
            break;
        }
    }

    if (surfaceFormat.format == vk::Format::eUndefined)
    {
        surfaceFormat = SwapChainDetails.Formats[0];
    }

    // Always guaranteed to be available.
    vk::PresentModeKHR presentMode = vk::PresentModeKHR::eFifo;

    const bool useMailboxPresentMode = std::any_of(SwapChainDetails.PresentModes.begin(),
                                                   SwapChainDetails.PresentModes.end(),
                                                   [](vk::PresentModeKHR InPresentMode)
                                                   {
                                                       // 'Triple Buffering'
                                                       return InPresentMode == vk::PresentModeKHR::eMailbox;
                                                   });

    if (useMailboxPresentMode)
    {
        presentMode = vk::PresentModeKHR::eMailbox;
    }

    SDL_Window* WindowHandle = SDL_GetWindowFromID(windowID);
    int32       Width, Height = 0;
    SDL_Vulkan_GetDrawableSize(WindowHandle, &Width, &Height);

    Width  = std::clamp(Width,
                       static_cast<int32>(SwapChainDetails.Capabilities.minImageExtent.width),
                       static_cast<int32>(SwapChainDetails.Capabilities.maxImageExtent.width));
    Height = std::clamp(Height,
                        static_cast<int32>(SwapChainDetails.Capabilities.minImageExtent.height),
                        static_cast<int32>(SwapChainDetails.Capabilities.maxImageExtent.height));

    uint32 ImageCount = SwapChainDetails.Capabilities.minImageCount + 1;
    if (SwapChainDetails.Capabilities.maxImageCount > 0)
    {
        ImageCount = std::min(ImageCount, SwapChainDetails.Capabilities.maxImageCount);
    }

    swapChainFormat = surfaceFormat;

    swapChainExtent = vk::Extent2D(Width, Height);

    vk::SwapchainCreateInfoKHR swapChainCreateInfo {.surface               = surface,
                                                    .minImageCount         = ImageCount,
                                                    .imageFormat           = swapChainFormat.format,
                                                    .imageColorSpace       = swapChainFormat.colorSpace,
                                                    .imageExtent           = swapChainExtent,
                                                    .imageArrayLayers      = 1,
                                                    .imageUsage            = vk::ImageUsageFlagBits::eColorAttachment,
                                                    .imageSharingMode      = vk::SharingMode::eExclusive,
                                                    .queueFamilyIndexCount = 0,
                                                    .pQueueFamilyIndices   = nullptr,
                                                    .preTransform          = SwapChainDetails.Capabilities.currentTransform,
                                                    .compositeAlpha        = vk::CompositeAlphaFlagBitsKHR::eOpaque,
                                                    .presentMode           = presentMode,
                                                    .clipped               = true};

    uint32 queueFamilyIndicesArray[] = {Indices.Graphics.value(), Indices.Present.value()};

    if (Indices.Graphics.value() != Indices.Present.value())
    {
        swapChainCreateInfo.imageSharingMode      = vk::SharingMode::eConcurrent;
        swapChainCreateInfo.queueFamilyIndexCount = ArrayLength(queueFamilyIndicesArray);
        swapChainCreateInfo.pQueueFamilyIndices   = ArrayData(queueFamilyIndicesArray);
    }

    swapChain = device.createSwapchainKHR(swapChainCreateInfo);
}

void Zn::VulkanDevice::CreateImageViews()
{
    swapChainImages = device.getSwapchainImagesKHR(swapChain);

    swapChainImageViews.resize(swapChainImages.size());

    for (size_t Index = 0; Index < swapChainImages.size(); ++Index)
    {
        vk::ImageViewCreateInfo imageViewCreateInfo {.image            = swapChainImages[Index],
                                                     .viewType         = vk::ImageViewType::e2D,
                                                     .format           = swapChainFormat.format,
                                                     // RGBA
                                                     .components       = {vk::ComponentSwizzle::eIdentity,
                                                                          vk::ComponentSwizzle::eIdentity,
                                                                          vk::ComponentSwizzle::eIdentity,
                                                                          vk::ComponentSwizzle::eIdentity},
                                                     .subresourceRange = {
                                                         .aspectMask     = vk::ImageAspectFlagBits::eColor,
                                                         .baseMipLevel   = 0,
                                                         .levelCount     = 1,
                                                         .baseArrayLayer = 0,
                                                         .layerCount     = 1,
                                                     }};

        swapChainImageViews[Index] = device.createImageView(imageViewCreateInfo);
    }

    // Initialize Depth Buffer

    if (auto result = textures.insert({depthTextureHandle,
                                       new RHITexture {.width  = static_cast<i32>(swapChainExtent.width),
                                                       .height = static_cast<i32>(swapChainExtent.height),
                                                       //	Hardcoding to 32 bit float.
                                                       //	Most GPUs support this depth format, so it’s fine to use it. You might want to
                                                       // choose other formats for other uses, or if you use Stencil buffer.
                                                       .format = vk::Format::eD32Sfloat}});
        result.second)
    {
        RHITexture* depthTexture = result.first->second;

        vk::Extent3D depthImageExtent {swapChainExtent.width, swapChainExtent.height, 1};

        //	VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT lets the vulkan driver know that this will be a depth image used for z-testing.
        vk::ImageCreateInfo depthImageCreateInfo =
            MakeImageCreateInfo(depthTexture->format, vk::ImageUsageFlagBits::eDepthStencilAttachment, depthImageExtent);

        //	Allocate from GPU memory.

        vma::AllocationCreateInfo depthImageAllocInfo {
            //	VMA_MEMORY_USAGE_GPU_ONLY to make sure that the image is allocated on fast VRAM.
            .usage         = vma::MemoryUsage::eGpuOnly,
            //	To make absolutely sure that VMA really allocates the image into VRAM, we give it VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT on
            // required flags.
            //	This forces VMA library to allocate the image on VRAM no matter what. (The Memory Usage part is more like a hint)
            .requiredFlags = vk::MemoryPropertyFlags(vk::MemoryPropertyFlagBits::eDeviceLocal),
        };

        ZN_VK_CHECK(
            allocator.createImage(&depthImageCreateInfo, &depthImageAllocInfo, &depthTexture->image, &depthTexture->allocation, nullptr));

        //	VK_IMAGE_ASPECT_DEPTH_BIT lets the vulkan driver know that this will be a depth image used for z-testing.
        vk::ImageViewCreateInfo depthImageViewCreateInfo =
            MakeImageViewCreateInfo(depthTexture->format, depthTexture->image, vk::ImageAspectFlagBits::eDepth);

        depthTexture->imageView = device.createImageView(depthImageViewCreateInfo);
    }
}

void Zn::VulkanDevice::CreateFramebuffers()
{
    // create the framebuffers for the swapchain images. This will connect the render-pass to the images for rendering
    vk::FramebufferCreateInfo framebufferCreateInfo {
        .renderPass = renderPass, .attachmentCount = 1, .width = swapChainExtent.width, .height = swapChainExtent.height, .layers = 1};

    // grab how many images we have in the swapchain
    const size_t numImages = swapChainImages.size();
    frameBuffers           = Vector<vk::Framebuffer>(numImages);

    RHITexture* depthTexture = textures[depthTextureHandle];

    // create framebuffers for each of the swapchain image views
    for (size_t index = 0; index < numImages; index++)
    {
        vk::ImageView attachments[2] = {swapChainImageViews[index], depthTexture->imageView};

        framebufferCreateInfo.setAttachments(attachments);

        frameBuffers[index] = device.createFramebuffer(framebufferCreateInfo);
    }
}

void Zn::VulkanDevice::CleanupSwapChain()
{
    for (size_t index = 0; index < frameBuffers.size(); ++index)
    {
        device.destroyFramebuffer(frameBuffers[index]);
        device.destroyImageView(swapChainImageViews[index]);
    }

    if (RHITexture* depthTexture = textures[depthTextureHandle])
    {
        device.destroyImageView(depthTexture->imageView);
        allocator.destroyImage(depthTexture->image, depthTexture->allocation);

        delete depthTexture;

        textures.erase(depthTextureHandle);
    }

    device.destroySwapchainKHR(swapChain);
}

void Zn::VulkanDevice::RecreateSwapChain()
{
    device.waitIdle();

    CleanupSwapChain();

    CreateSwapChain();

    CreateImageViews();

    CreateFramebuffers();
}

void Zn::VulkanDevice::SetViewport(vk::CommandBuffer cmd)
{
    static constexpr f32 ratio = 16.f / 9.f;

    f32 width  = static_cast<float>(swapChainExtent.width);
    f32 height = static_cast<float>(swapChainExtent.height);

    if (width / height > ratio)
    {
        width = (height * ratio);
    }
    else if (width / height < ratio)
    {
        height = (width / ratio);
    }

    // Compute offset to always render in the center.
    const f32 xOffset = (swapChainExtent.width - width) * 0.5f;
    const f32 yOffset = (swapChainExtent.height - height) * 0.5f;

    vk::Viewport viewport {
        .x        = xOffset,
        .y        = yOffset,
        .width    = width,
        .height   = height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };

    vk::Rect2D scissors {
        .offset = vk::Offset2D {.x = static_cast<i32>(xOffset), .y = static_cast<i32>(yOffset)},
        .extent = swapChainExtent,
    };

    cmd.setViewport(0, {viewport});
    cmd.setScissor(0, {scissors});
}

RHIBuffer Zn::VulkanDevice::CreateBuffer(size_t                     size,
                                         vk::BufferUsageFlags       usage,
                                         vma::MemoryUsage           memoryUsage,
                                         vma::AllocationCreateFlags allocationFlags) const
{
    vk::BufferCreateInfo createInfo {.size = size, .usage = usage};

    vma::AllocationCreateInfo allocationInfo {
        .flags         = allocationFlags,
        .usage         = memoryUsage,
        .requiredFlags = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
    };

    return RHIBuffer(allocator.createBuffer(createInfo, allocationInfo));
}

void Zn::VulkanDevice::DestroyBuffer(RHIBuffer buffer) const
{
    if (buffer)
    {
        allocator.destroyBuffer(buffer.data, buffer.allocation);
    }
}

void Zn::VulkanDevice::CopyToGPU(vma::Allocation allocation, const void* src, size_t size) const
{
    void* dst = allocator.mapMemory(allocation);

    memcpy(dst, src, size);

    allocator.unmapMemory(allocation);
}

RHIMesh* Zn::VulkanDevice::GetMesh(const String& InName)
{
    if (auto it = meshes.find(HashCalculate(InName)); it != meshes.end())
    {
        return it->second;
    }

    return nullptr;
}

void Zn::VulkanDevice::DrawObjects(vk::CommandBuffer commandBuffer, RenderObject* first, u64 count)
{
    ZN_TRACE_QUICKSCOPE();

    SetViewport(commandBuffer);

    for (u64 index = 0; index < count; ++index)
    {
        RenderObject* current = first + index;

        commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, current->material->pipeline);

        u32 offset = 0;

        // Global Descriptor Set
        commandBuffer.bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics, current->material->layout, 0, globalDescriptorSets[currentFrame], {});

        // Primitive/Material Descriptor Set
        commandBuffer.bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics, current->material->layout, 1, gpuPrimitivesDescriptorSets[current->primitive], {});

        commandBuffer.bindVertexBuffers(0,
                                        {
                                            current->primitive->position.data,
                                            current->primitive->normal.data,
                                            current->primitive->tangent.data,
                                            current->primitive->uv.data,
                                            // current->primitive->color.data,
                                        },
                                        {offset, offset, offset, offset});

        bool isIndexedDraw = current->primitive->numIndices > 0;

        if (isIndexedDraw)
        {
            // TODO: Save index type into primitive?
            commandBuffer.bindIndexBuffer(current->primitive->indices.data, offset, vk::IndexType::eUint32);
        }

        static const auto identity = glm::mat4 {1.f};

        glm::mat4 translation = glm::translate(identity, current->location);
        glm::mat4 rotation    = glm::mat4_cast(current->rotation);
        glm::mat4 scale       = glm::scale(identity, current->scale);

        MeshPushConstants constants {
            .model         = translation * rotation * scale,
            .model_inverse = glm::inverse(translation * rotation * scale),
        };

        commandBuffer.pushConstants(current->material->layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(MeshPushConstants), &constants);

        if (isIndexedDraw)
        {
            commandBuffer.drawIndexed(current->primitive->numIndices, 1, 0, 0, 0);
        }
        else
        {
            commandBuffer.draw(current->primitive->numVertices, 1, 0, 0);
        }
    }
    /*if (gpuFeatures.multiDrawIndirect)
    {
        IndirectDrawBatch drawBatches[64] = {0};

        i32 batchCount = -1;

        {
            RHIMesh*  lastMesh     = nullptr;
            Material* lastMaterial = nullptr;

            for (u32 index = 0; index < count; ++index)
            {
                RenderObject& object = first[index];

                if (object.material != lastMaterial || lastMesh != object.mesh)
                {
                    ++batchCount;

                    IndirectDrawBatch& batch = drawBatches[batchCount];

                    batch = IndirectDrawBatch {
                        .material = object.material,
                        .mesh     = object.mesh,
                        .index    = index,
                        .count    = 1,
                    };

                    lastMesh     = object.mesh;
                    lastMaterial = object.material;
                }
                else
                {
                    ++drawBatches[batchCount].count;
                }
            }
        }

        vk::DrawIndexedIndirectCommand* dst =
            reinterpret_cast<vk::DrawIndexedIndirectCommand*>(allocator.mapMemory(drawCommands[currentFrame].allocation));

        for (u64 index = 0; index < count; ++index)
        {
            RenderObject& object = first[index];

            dst[index] = vk::DrawIndexedIndirectCommand {
                .indexCount    = static_cast<u32>(object.mesh->indices.size()),
                .instanceCount = 1,
                .firstIndex    = 0,
                .vertexOffset  = 0,
                .firstInstance = static_cast<u32>(index),
            };
        }

        allocator.unmapMemory(drawCommands[currentFrame].allocation);

        SetViewport(commandBuffer);

        for (i32 batchIndex = 0; batchIndex < batchCount + 1; ++batchIndex)
        {
            const IndirectDrawBatch& batch = drawBatches[batchIndex];

            commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, batch.material->pipeline);

            u32 globalOffset = sizeof(RHIInstanceData) * batch.count;
            commandBuffer.bindDescriptorSets(
                vk::PipelineBindPoint::eGraphics, batch.material->layout, 0, globalDescriptorSets[currentFrame], {globalOffset});

            if (batch.material->textureSet)
            {
                commandBuffer.bindDescriptorSets(
                    vk::PipelineBindPoint::eGraphics, batch.material->layout, 1, batch.material->textureSet, {});
            }

            vk::DeviceSize offset = 0;
            commandBuffer.bindVertexBuffers(0, 1, &batch.mesh->vertexBuffer.data, &offset);

            commandBuffer.bindVertexBuffers(1, 1, &instanceData[currentFrame].data, &offset);

            if (batch.mesh->indexBuffer.data)
            {
                commandBuffer.bindIndexBuffer(batch.mesh->indexBuffer.data, 0, vk::IndexType::eUint32);
            }

            vk::DeviceSize indirectOffset = batch.index * sizeof(vk::DrawIndexedIndirectCommand);
            u32            stride         = sizeof(vk::DrawIndexedIndirectCommand);

            commandBuffer.drawIndexedIndirect(drawCommands[currentFrame].data, indirectOffset, batch.count, stride);
        }
    }*/
}

void Zn::VulkanDevice::CreateScene()
{
    Material* basePbr = VulkanMaterialManager::Get().GetMaterial("base_pbr");

    vk::DescriptorSetAllocateInfo materialSetAllocateInfo {
        .descriptorPool     = descriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts        = &basePbr->materialSetLayout,
    };

    for (RHIPrimitiveGPU* primitive : gpuPrimitives)
    {
        vk::DescriptorSet perPrimitiveSet = device.allocateDescriptorSets(materialSetAllocateInfo)[0];

        // Vector<std::pair<vk::DescriptorImageInfo, u32>> descriptorImages;
        Vector<vk::DescriptorImageInfo> descriptorImages;
        descriptorImages.resize(5);

        auto InsertTexture = [&](cstring textureTypeName, ResourceHandle& textureHandle, u32 index, const String& defaultTexture) -> bool
        {
            RHITexture* texture = nullptr;
            if (auto it = textures.find(textureHandle); textureHandle && it != textures.end())
            {
                texture = it->second;
            }
            else if (defaultTexture.length() > 0)
            {
                ZN_LOG(LogVulkan, ELogVerbosity::Warning, "Missing %s. Replacing with %s", textureTypeName, defaultTexture.c_str());
                ResourceHandle defaultHandle = ResourceHandle(defaultTexture);
                auto           it            = textures.find(defaultHandle);
                if (it != textures.end())
                {
                    texture       = it->second;
                    textureHandle = defaultHandle;
                }
            }

            if (texture)
            {
                vk::DescriptorImageInfo textureInfo {
                    .sampler     = texture->sampler,
                    .imageView   = texture->imageView,
                    .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
                };

                // descriptorImages.push_back({std::move(textureInfo), index});
                descriptorImages[index] = std::move(textureInfo);
                return true;
            }
            return false;
        };

        // TODO: We need to create and assign default textures.

        InsertTexture("baseColor", primitive->materialAttributes.baseColorTexture, 0, uvCheckerTexturePath);
        InsertTexture("metalness", primitive->materialAttributes.metalnessTexture, 1, blackTextureName);
        InsertTexture("normal", primitive->materialAttributes.normalTexture, 2, normalTextureName);
        InsertTexture("occlusion", primitive->materialAttributes.occlusionTexture, 3, blackTextureName);
        InsertTexture("emissive", primitive->materialAttributes.emissiveTexture, 4, blackTextureName);

        vk::WriteDescriptorSet writeOperations[2] = {{
                                                         .dstSet          = perPrimitiveSet,
                                                         .dstBinding      = 0,
                                                         .dstArrayElement = 0,
                                                         .descriptorCount = 5,
                                                         .descriptorType  = vk::DescriptorType::eCombinedImageSampler,
                                                         //.pImageInfo      = &imageInfo.first,
                                                         .pImageInfo      = descriptorImages.data(),
                                                     },
                                                     {}};

        // Vector<vk::WriteDescriptorSet> writeOperations;

        // for (const std::pair<vk::DescriptorImageInfo, u32>& imageInfo : descriptorImages)
        //{
        //     writeOperations.push_back(vk::WriteDescriptorSet {
        //         .dstSet          = perPrimitiveSet,
        //         .dstBinding      = 0,
        //         .dstArrayElement = imageInfo.second,
        //         .descriptorCount = 1,
        //         .descriptorType  = vk::DescriptorType::eCombinedImageSampler,
        //         .pImageInfo      = &imageInfo.first,
        //     });
        // }

        vk::DescriptorBufferInfo uboBufferInfo {
            .buffer = primitive->uboMaterialAttributes.data,
            .offset = 0,
            .range  = VK_WHOLE_SIZE,
        };
        vk::WriteDescriptorSet uboWrite {
            .dstSet          = perPrimitiveSet,
            .dstBinding      = 1,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType  = vk::DescriptorType::eUniformBuffer,
            .pBufferInfo     = &uboBufferInfo,
        };

        // writeOperations.push_back(uboWrite);
        writeOperations[1] = uboWrite;

        device.updateDescriptorSets(writeOperations, {});

        gpuPrimitivesDescriptorSets.insert({primitive, perPrimitiveSet});

        RenderObject entity {
            .primitive = primitive,
            .material  = basePbr,
            .location  = glm::vec3(0.f),
            .rotation  = glm::quat(),
            .scale     = glm::vec3(1.f),
        };

        renderables.push_back(std::move(entity));
    }
}

void Zn::VulkanDevice::CreateDefaultTexture(const String& name, const u8 (&color)[4])
{
    RHITexture* defaultTexture = CreateRHITexture(1, 1, vk::Format::eR8G8B8A8Unorm);

    RHIBuffer stagingBuffer = CreateBuffer(1 * sizeof(u8) * 4, vk::BufferUsageFlagBits::eTransferSrc, vma::MemoryUsage::eCpuOnly);

    CopyToGPU(stagingBuffer.allocation, ArrayData(color), ArrayLength(color));

    ImmediateSubmit(
        [=](vk::CommandBuffer cmd)
        {
            TransitionImageLayout(
                cmd, defaultTexture->image, vk::Format::eR8G8B8A8Unorm, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
            CopyBufferToImage(cmd, stagingBuffer.data, defaultTexture->image, 1, 1);
            TransitionImageLayout(cmd,
                                  defaultTexture->image,
                                  vk::Format::eR8G8B8A8Unorm,
                                  vk::ImageLayout::eTransferDstOptimal,
                                  vk::ImageLayout::eShaderReadOnlyOptimal);
        });

    DestroyBuffer(stagingBuffer);

    vk::ImageViewCreateInfo imageViewInfo {.image            = defaultTexture->image,
                                           .viewType         = vk::ImageViewType::e2D,
                                           .format           = vk::Format::eR8G8B8A8Unorm,
                                           .subresourceRange = {
                                               .aspectMask     = vk::ImageAspectFlagBits::eColor,
                                               .baseMipLevel   = 0,
                                               .levelCount     = 1,
                                               .baseArrayLayer = 0,
                                               .layerCount     = 1,
                                           }};

    defaultTexture->imageView = device.createImageView(imageViewInfo);

    defaultTexture->sampler = CreateSampler(defaultTextureSampler);

    VulkanValidation::SetObjectDebugName(device, name.c_str(), defaultTexture->image);

    textures.insert({ResourceHandle(name), defaultTexture});
}

void Zn::VulkanDevice::CreateDefaultResources()
{
    u8 white[4] = {255, 255, 255, 255};
    CreateDefaultTexture(whiteTextureName, white);
    u8 black[4] = {0, 0, 0, 255};
    CreateDefaultTexture(blackTextureName, black);
    u8 normal[4] = {128, 128, 255, 255};
    CreateDefaultTexture(normalTextureName, normal);

    RHITexture* uvChecker = CreateTexture(uvCheckerTexturePath);
    uvChecker->sampler    = CreateSampler(defaultTextureSampler);

    VulkanValidation::SetObjectDebugName(device, uvCheckerName.c_str(), uvChecker->image);
}

void Zn::VulkanDevice::LoadMeshes()
{
    String gltfModelPath = gltfDefaultModel;

    if (String gltfSampleModelName = GetGLTFSampleModel(); !gltfSampleModelName.empty())
    {
        gltfModelPath = gltfSampleModels + '/' + gltfSampleModelName + "/glTF/" + gltfSampleModelName + ".gltf";
    }

    MeshImporterOutput gltfOutput;
    if (MeshImporter::ImportAll(IO::GetAbsolutePath(gltfModelPath), gltfOutput))
    {
        ZN_TRACE_QUICKSCOPE();

        // TODO: Create Buffers
        // TODO: Create Materials
        for (const auto& it : gltfOutput.textures)
        {
            RHITexture* texture = CreateTexture(it.first, it.second);

            if (auto samplerIt = gltfOutput.samplers.find(it.first); samplerIt != gltfOutput.samplers.end())
            {
                texture->sampler = CreateSampler(samplerIt->second);
            }
        }

        i32 index = 0;

        for (const RHIPrimitive& cpuPrimitive : gltfOutput.primitives)
        {
            ZN_TRACE_QUICKSCOPE();
            RHIPrimitiveGPU* gpuPrimitive = new RHIPrimitiveGPU();

            // TODO: Very inefficient to create and submit buffers one by one.

            gpuPrimitive->numVertices = cpuPrimitive.position.size();
            gpuPrimitive->numIndices  = cpuPrimitive.indices.size();

            gpuPrimitive->position = CreateRHIBuffer(cpuPrimitive.position.data(),
                                                     cpuPrimitive.position.size() * sizeof(glm::vec3),
                                                     vk::BufferUsageFlagBits::eVertexBuffer,
                                                     vma::MemoryUsage::eGpuOnly);
            if (cpuPrimitive.normal.size() > 0)
            {
                gpuPrimitive->normal = CreateRHIBuffer(cpuPrimitive.normal.data(),
                                                       cpuPrimitive.normal.size() * sizeof(glm::vec3),
                                                       vk::BufferUsageFlagBits::eVertexBuffer,
                                                       vma::MemoryUsage::eGpuOnly);
            }
            else
            {
                ZN_LOG(LogVulkan, ELogVerbosity::Warning, "Unable to find Normal buffer for primitive. Creating default.");
                Vector<glm::vec3> dummy;
                dummy.resize(cpuPrimitive.position.size());

                memset(dummy.data(), 0, dummy.size() * sizeof(glm::vec3));

                gpuPrimitive->normal = CreateRHIBuffer(
                    dummy.data(), dummy.size() * sizeof(glm::vec3), vk::BufferUsageFlagBits::eVertexBuffer, vma::MemoryUsage::eGpuOnly);
            }

            if (cpuPrimitive.tangent.size() > 0)
            {
                gpuPrimitive->tangent = CreateRHIBuffer(cpuPrimitive.tangent.data(),
                                                        cpuPrimitive.tangent.size() * sizeof(glm::vec3),
                                                        vk::BufferUsageFlagBits::eVertexBuffer,
                                                        vma::MemoryUsage::eGpuOnly);
            }
            else
            {
                ZN_LOG(LogVulkan, ELogVerbosity::Warning, "Unable to find Tangent buffer for primitive. Creating default.");
                Vector<glm::vec3> dummy;
                dummy.resize(cpuPrimitive.position.size());

                memset(dummy.data(), 0, dummy.size() * sizeof(glm::vec3));

                gpuPrimitive->tangent = CreateRHIBuffer(
                    dummy.data(), dummy.size() * sizeof(glm::vec3), vk::BufferUsageFlagBits::eVertexBuffer, vma::MemoryUsage::eGpuOnly);
            }

            if (cpuPrimitive.uv.size() > 0)
            {
                gpuPrimitive->uv = CreateRHIBuffer(cpuPrimitive.uv.data(),
                                                   cpuPrimitive.uv.size() * sizeof(glm::vec2),
                                                   vk::BufferUsageFlagBits::eVertexBuffer,
                                                   vma::MemoryUsage::eGpuOnly);
            }

            // if (cpuPrimitive.color.size() > 0)
            //{
            //     gpuPrimitive->color = CreateRHIBuffer(cpuPrimitive.color.data(),
            //                                           cpuPrimitive.color.size() * sizeof(glm::vec4),
            //                                           vk::BufferUsageFlagBits::eVertexBuffer,
            //                                           vma::MemoryUsage::eGpuOnly);
            // }

            if (cpuPrimitive.indices.size() > 0)
            {
                gpuPrimitive->indices = CreateRHIBuffer(cpuPrimitive.indices.data(),
                                                        cpuPrimitive.indices.size() * sizeof(u32),
                                                        vk::BufferUsageFlagBits::eIndexBuffer,
                                                        vma::MemoryUsage::eGpuOnly);
            }

            gpuPrimitive->materialAttributes = cpuPrimitive.materialAttributes;

            gpuPrimitive->uboMaterialAttributes =
                CreateBuffer(sizeof(UBOMaterialAttributes),
                             vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferDst,
                             vma::MemoryUsage::eGpuOnly);

            RHIBuffer stagingBuffer =
                CreateBuffer(sizeof(UBOMaterialAttributes), vk::BufferUsageFlagBits::eTransferSrc, vma::MemoryUsage::eCpuOnly);

            UBOMaterialAttributes uboMaterialAttributes {
                .baseColor   = gpuPrimitive->materialAttributes.baseColor,
                .metalness   = gpuPrimitive->materialAttributes.metalness,
                .roughness   = gpuPrimitive->materialAttributes.roughness,
                .alphaCutoff = gpuPrimitive->materialAttributes.alphaCutoff,
                .emissive    = gpuPrimitive->materialAttributes.emissive,
            };

            CopyToGPU(stagingBuffer.allocation, &uboMaterialAttributes, sizeof(UBOMaterialAttributes));

            ImmediateSubmit(
                [=](vk::CommandBuffer cmd)
                {
                    cmd.copyBuffer(stagingBuffer.data,
                                   gpuPrimitive->uboMaterialAttributes.data,
                                   {vk::BufferCopy(0, 0, sizeof(UBOMaterialAttributes))});
                });

            DestroyBuffer(stagingBuffer);

            gpuPrimitives.emplace_back(gpuPrimitive);
        }
    }
}

RHIBuffer Zn::VulkanDevice::CreateRHIBuffer(const void*          data,
                                            sizet                size,
                                            vk::BufferUsageFlags bufferUsage,
                                            vma::MemoryUsage     memoryUsage) const
{
    RHIBuffer stagingBuffer = CreateBuffer(size, vk::BufferUsageFlagBits::eTransferSrc, vma::MemoryUsage::eCpuOnly);

    CopyToGPU(stagingBuffer.allocation, data, size);

    RHIBuffer outputBuffer = CreateBuffer(size, bufferUsage | vk::BufferUsageFlagBits::eTransferDst, memoryUsage);

    ImmediateSubmit(
        [=](vk::CommandBuffer cmd)
        {
            cmd.copyBuffer(stagingBuffer.data, outputBuffer.data, {vk::BufferCopy(0, 0, size)});
        });

    DestroyBuffer(stagingBuffer);

    return outputBuffer;
}

void Zn::VulkanDevice::CreateMeshPipeline()
{
    Vector<uint8> vertex_shader_data;
    Vector<uint8> fragment_shader_data;

    String vertexShaderName;
    String fragmentShaderName;

    CommandLine::Get().Value("-vertex", vertexShaderName, "vertex");
    CommandLine::Get().Value("-fragment", fragmentShaderName, "fragment");

    vertexShaderName   = "shaders/" + vertexShaderName + ".vert.spv";
    fragmentShaderName = "shaders/" + fragmentShaderName + ".frag.spv";

    const bool vertex_success   = IO::ReadBinaryFile(vertexShaderName, vertex_shader_data);
    const bool fragment_success = IO::ReadBinaryFile(fragmentShaderName, fragment_shader_data);

    if (vertex_success && fragment_success)
    {
        Material* material = VulkanMaterialManager::Get().CreateMaterial("base_pbr");

        material->vertexShader   = CreateShaderModule(vertex_shader_data);
        material->fragmentShader = CreateShaderModule(fragment_shader_data);

        if (!material->vertexShader)
        {
            ZN_LOG(LogVulkan, ELogVerbosity::Error, "Failed to create vertex shader.");
        }

        if (!material->fragmentShader)
        {
            ZN_LOG(LogVulkan, ELogVerbosity::Error, "Failed to create fragment shader.");
        }

        vk::DescriptorSetLayoutBinding pbrTextureSetBinding {
            .binding         = 0,
            .descriptorType  = vk::DescriptorType::eCombinedImageSampler,
            .descriptorCount = 5,
            .stageFlags      = vk::ShaderStageFlagBits::eFragment,
        };

        vk::DescriptorSetLayoutBinding pbrMaterialAttributesSetBinding {
            .binding         = 1,
            .descriptorType  = vk::DescriptorType::eUniformBuffer,
            .descriptorCount = 1,
            .stageFlags      = vk::ShaderStageFlagBits::eFragment,
        };

        vk::DescriptorSetLayoutBinding pbrBindings[2] = {pbrTextureSetBinding, pbrMaterialAttributesSetBinding};

        vk::DescriptorSetLayoutCreateInfo pbrTexturesSetCreateInfo {
            .bindingCount = ArrayLength(pbrBindings),
            .pBindings    = ArrayData(pbrBindings),
        };

        material->materialSetLayout = device.createDescriptorSetLayout(pbrTexturesSetCreateInfo);

        // TODO: each material should destroy its own when dealocated?
        destroyQueue.Enqueue(
            [=]()
            {
                device.destroyDescriptorSetLayout(material->materialSetLayout);
            });

        vk::DescriptorSetLayout layouts[2] = {globalDescriptorSetLayout, material->materialSetLayout};

        vk::PipelineLayoutCreateInfo layoutCreateInfo {.setLayoutCount = ArrayLength(layouts), .pSetLayouts = ArrayData(layouts)};

        vk::PushConstantRange pushConstants {
            // this push constant range is accessible only in the vertex shader
            .stageFlags = vk::ShaderStageFlagBits::eVertex,
            .offset     = 0,
            .size       = sizeof(MeshPushConstants),
        };

        layoutCreateInfo.setPushConstantRanges(pushConstants);

        material->layout = device.createPipelineLayout(layoutCreateInfo);

        destroyQueue.Enqueue(
            [=]()
            {
                device.destroyPipelineLayout(material->layout);
            });

        material->pipeline = VulkanPipeline::NewVkPipeline(device,
                                                           renderPass,
                                                           material->vertexShader,
                                                           material->fragmentShader,
                                                           swapChainExtent,
                                                           material->layout,
                                                           VulkanPipeline::gltfInputLayout);

        destroyQueue.Enqueue(
            [=]()
            {
                device.destroyShaderModule(material->vertexShader);
                device.destroyShaderModule(material->fragmentShader);
                device.destroyPipeline(material->pipeline);
            });
    }
}

RHITexture* Zn::VulkanDevice::CreateTexture(const String& path)
{
    ResourceHandle textureHandle = HashCalculate(path);

    if (auto it = textures.find(textureHandle); it != std::end(textures))
    {
        return it->second;
    }

    RHIBuffer stagingBuffer {};

    i32 width  = 0;
    i32 height = 0;

    if (SharedPtr<TextureSource> sourceTexture = TextureImporter::Import(IO::GetAbsolutePath(path)))
    {
        width  = sourceTexture->width;
        height = sourceTexture->height;

        stagingBuffer = CreateBuffer(sourceTexture->data.size(), vk::BufferUsageFlagBits::eTransferSrc, vma::MemoryUsage::eCpuOnly);
        CopyToGPU(stagingBuffer.allocation, sourceTexture->data.data(), sourceTexture->data.size());
    }
    else
    {
        ZN_LOG(LogVulkan, ELogVerbosity::Warning, "Failed to load texture %s", path.c_str());
        return nullptr;
    }

    RHITexture* texture = nullptr;

    if (auto result = textures.insert({textureHandle, CreateRHITexture(width, height, vk::Format::eR8G8B8A8Srgb)}); result.second)
    {
        texture = result.first->second;
    }

    ImmediateSubmit(
        [=](vk::CommandBuffer cmd)
        {
            TransitionImageLayout(
                cmd, texture->image, vk::Format::eR8G8B8A8Srgb, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
            CopyBufferToImage(cmd, stagingBuffer.data, texture->image, width, height);
            TransitionImageLayout(cmd,
                                  texture->image,
                                  vk::Format::eR8G8B8A8Srgb,
                                  vk::ImageLayout::eTransferDstOptimal,
                                  vk::ImageLayout::eShaderReadOnlyOptimal);
        });

    DestroyBuffer(stagingBuffer);

    vk::ImageViewCreateInfo imageViewInfo {.image            = texture->image,
                                           .viewType         = vk::ImageViewType::e2D,
                                           .format           = vk::Format::eR8G8B8A8Srgb,
                                           .subresourceRange = {
                                               .aspectMask     = vk::ImageAspectFlagBits::eColor,
                                               .baseMipLevel   = 0,
                                               .levelCount     = 1,
                                               .baseArrayLayer = 0,
                                               .layerCount     = 1,
                                           }};

    texture->imageView = device.createImageView(imageViewInfo);

    VulkanValidation::SetObjectDebugName(device, path.c_str(), texture->image);

    return texture;
}

RHITexture* Zn::VulkanDevice::CreateTexture(const String& name, SharedPtr<TextureSource> texture)
{
    ResourceHandle textureHandle = HashCalculate(name);

    if (auto it = textures.find(textureHandle); it != std::end(textures))
    {
        return it->second;
    }

    i32 width  = texture->width;
    i32 height = texture->height;

    RHIBuffer stagingBuffer = CreateBuffer(texture->data.size(), vk::BufferUsageFlagBits::eTransferSrc, vma::MemoryUsage::eCpuOnly);
    CopyToGPU(stagingBuffer.allocation, texture->data.data(), texture->data.size());

    RHITexture* rhiTexture = nullptr;

    // TODO: This should be derived from the source maybe?
    const vk::Format textureFormat = vk::Format::eR8G8B8A8Srgb;

    if (auto result = textures.insert({textureHandle, CreateRHITexture(width, height, textureFormat)}); result.second)
    {
        rhiTexture = result.first->second;
    }

    ImmediateSubmit(
        [=](vk::CommandBuffer cmd)
        {
            TransitionImageLayout(cmd, rhiTexture->image, textureFormat, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
            CopyBufferToImage(cmd, stagingBuffer.data, rhiTexture->image, width, height);
            TransitionImageLayout(
                cmd, rhiTexture->image, textureFormat, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
        });

    DestroyBuffer(stagingBuffer);

    vk::ImageViewCreateInfo imageViewInfo {.image            = rhiTexture->image,
                                           .viewType         = vk::ImageViewType::e2D,
                                           .format           = textureFormat,
                                           .subresourceRange = {
                                               .aspectMask     = vk::ImageAspectFlagBits::eColor,
                                               .baseMipLevel   = 0,
                                               .levelCount     = 1,
                                               .baseArrayLayer = 0,
                                               .layerCount     = 1,
                                           }};

    rhiTexture->imageView = device.createImageView(imageViewInfo);
    rhiTexture->format    = textureFormat;

    VulkanValidation::SetObjectDebugName(device, name.c_str(), rhiTexture->image);

    return rhiTexture;
}

RHITexture* Zn::VulkanDevice::CreateRHITexture(i32 width, i32 height, vk::Format format) const
{
    vk::ImageCreateInfo createInfo = MakeImageCreateInfo(format,
                                                         vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
                                                         vk::Extent3D {static_cast<u32>(width), static_cast<u32>(height), 1});

    createInfo.initialLayout = vk::ImageLayout::eUndefined;
    createInfo.sharingMode   = vk::SharingMode::eExclusive;

    //	Allocate from GPU memory.

    vma::AllocationCreateInfo allocationInfo {
        //	VMA_MEMORY_USAGE_GPU_ONLY to make sure that the image is allocated on fast VRAM.
        .usage         = vma::MemoryUsage::eGpuOnly,
        //	To make absolutely sure that VMA really allocates the image into VRAM, we give it VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT on
        // required flags.
        //	This forces VMA library to allocate the image on VRAM no matter what. (The Memory Usage part is more like a hint)
        .requiredFlags = vk::MemoryPropertyFlags(vk::MemoryPropertyFlagBits::eDeviceLocal)};

    RHITexture* texture = new RHITexture {.width = width, .height = height};

    ZN_VK_CHECK(allocator.createImage(&createInfo, &allocationInfo, &texture->image, &texture->allocation, nullptr));

    return texture;
}

vk::Sampler Zn::VulkanDevice::CreateSampler(const TextureSampler& sampler)
{
    vk::SamplerCreateInfo samplerCreateInfo {
        .magFilter    = TranslateSamplerFilter(sampler.magnification),
        .minFilter    = TranslateSamplerFilter(sampler.minification),
        .addressModeU = TranslateSamplerWrap(sampler.wrapUV[0]),
        .addressModeV = TranslateSamplerWrap(sampler.wrapUV[1]),
        .addressModeW = TranslateSamplerWrap(sampler.wrapUV[2]),
    };

    return device.createSampler(samplerCreateInfo);
}

void Zn::VulkanDevice::TransitionImageLayout(
    vk::CommandBuffer cmd, vk::Image img, vk::Format fmt, vk::ImageLayout prevLayout, vk::ImageLayout newLayout) const
{
    vk::ImageMemoryBarrier barrier {
        .oldLayout           = prevLayout,
        .newLayout           = newLayout,
        // If you are using the barrier to transfer queue family ownership, then these two fields should be the indices of the queue
        // families. They must be set to VK_QUEUE_FAMILY_IGNORED if you don't want to do this (not the default value!).
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image               = img,
        .subresourceRange    = {
               .aspectMask     = vk::ImageAspectFlagBits::eColor,
               .baseMipLevel   = 0,
               .levelCount     = 1,
               .baseArrayLayer = 0,
               .layerCount     = 1,
        }};

    vk::PipelineStageFlags srcStage {};
    vk::PipelineStageFlags dstStage {};

    if (prevLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal)
    {
        barrier.srcAccessMask = vk::AccessFlags(0);
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

        srcStage = vk::PipelineStageFlagBits::eTopOfPipe;
        dstStage = vk::PipelineStageFlagBits::eTransfer;
    }
    else if (prevLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal)
    {
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

        srcStage = vk::PipelineStageFlagBits::eTransfer;
        dstStage = vk::PipelineStageFlagBits::eFragmentShader;
    }
    else
    {
        _ASSERT(false); // Unsupported Transition
    }

    cmd.pipelineBarrier(srcStage, dstStage, vk::DependencyFlags(0), {}, {}, {barrier});
}

void Zn::VulkanDevice::ImmediateSubmit(std::function<void(vk::CommandBuffer)>&& function) const
{
    if (function == nullptr)
    {
        ZN_LOG(LogVulkan, ELogVerbosity::Warning, "Trying to ImmediateSubmit with invalid function");
        return;
    }

    vk::CommandBuffer cmd = uploadContext.cmdBuffer;

    cmd.begin({.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});

    {
        ZN_TRACE_GPU_SCOPE("Submit", uploadContext.traceContext, cmd);
        function(cmd);
    }

    cmd.end();

    vk::SubmitInfo submitInfo {.commandBufferCount = 1, .pCommandBuffers = &cmd};

    // .fence will now block until the graphic commands finish execution
    graphicsQueue.submit(submitInfo, uploadContext.fence);

    ZN_VK_CHECK(device.waitForFences({uploadContext.fence}, true, kWaitTimeOneSecond));
    device.resetFences({uploadContext.fence});

    ZN_TRACE_GPU_COLLECT(uploadContext.traceContext, uploadContext.cmdBuffer);

    // reset the command buffers inside the command pool
    device.resetCommandPool(uploadContext.commandPool, vk::CommandPoolResetFlags(0));
}

vk::ImageCreateInfo Zn::VulkanDevice::MakeImageCreateInfo(vk::Format format, vk::ImageUsageFlags usageFlags, vk::Extent3D extent) const
{
    //	.format = texture data type, like holding a single float(for depth), or holding color.
    //	.extent = size of the image, in pixels.
    //	.mipLevels = num of mipmap levels the image has. TODO: Because we aren’t using them here, we leave the levels to 1.
    //	.arrayLayers = used for layered textures.
    //					You can create textures that are many - in - one, using layers.
    //					An example of layered textures is cubemaps, where you have 6 layers, one layer for each face of the cubemap.
    //					We default it to 1 layer because we aren’t doing cubemaps.
    //	.samples = controls the MSAA behavior of the texture. This only makes sense for render targets, such as depth images and images you
    // are rendering
    // to.
    //					TODO: We won’t be doing MSAA in this tutorial, so samples will be kept at 1 sample.
    //  .tiling = if you use VK_IMAGE_TILING_OPTIMAL, it won’t be possible to read the data from CPU or to write it without changing its
    //  tiling first
    //					(it’s possible to change the tiling of a texture at any point, but this can be a costly operation).
    //					The other tiling we can care about is VK_IMAGE_TILING_LINEAR, which will store the image as a 2d array of pixels.
    //	.usage = controls how the GPU handles the image memory.

    return {.imageType   = vk::ImageType::e2D,
            .format      = format,
            .extent      = extent,
            .mipLevels   = 1,
            .arrayLayers = 1,
            .samples     = vk::SampleCountFlagBits::e1,
            .tiling      = vk::ImageTiling::eOptimal,
            .usage       = usageFlags};
}
vk::ImageViewCreateInfo Zn::VulkanDevice::MakeImageViewCreateInfo(vk::Format              format,
                                                                  vk::Image               image,
                                                                  vk::ImageAspectFlagBits aspectFlags) const
{
    return {.image            = image,
            //	While imageType held the dimensionality of the texture, viewType has a lot more options, like VK_IMAGE_VIEW_TYPE_CUBE for
            // cubemaps.
            .viewType         = vk::ImageViewType::e2D,
            //	TODO: In here, we will have it matched to GetImageCreateInfo, and hardcode it to 2D images as it’s the most common case.
            .format           = format,
            .subresourceRange = {
                .aspectMask     = aspectFlags,
                .baseMipLevel   = 0,
                .levelCount     = 1,
                .baseArrayLayer = 0,
                .layerCount     = 1,
            }};
}

void Zn::VulkanDevice::CopyBufferToImage(vk::CommandBuffer cmd, vk::Buffer buffer, vk::Image img, u32 width, u32 height) const
{
    vk::BufferImageCopy region {.bufferOffset      = 0,
                                .bufferRowLength   = 0,
                                .bufferImageHeight = 0,
                                .imageSubresource =
                                    {
                                        .aspectMask     = vk::ImageAspectFlagBits::eColor,
                                        .mipLevel       = 0,
                                        .baseArrayLayer = 0,
                                        .layerCount     = 1,
                                    },
                                .imageOffset = vk::Offset3D {0, 0, 0},
                                .imageExtent = vk::Extent3D {width, height, 1}};

    cmd.copyBufferToImage(buffer, img, vk::ImageLayout::eTransferDstOptimal, {region});
}

VulkanDevice::DestroyQueue::~DestroyQueue()
{
    Flush();
}

void Zn::VulkanDevice::DestroyQueue::Enqueue(std::function<void()>&& InDestructor)
{
    queue.emplace_back(std::move(InDestructor));
}

void Zn::VulkanDevice::DestroyQueue::Flush()
{
    while (!queue.empty())
    {
        std::invoke(queue.front());

        queue.pop_front();
    }
}
