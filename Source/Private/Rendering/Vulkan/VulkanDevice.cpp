#include <Znpch.h>
#include <Core/Containers/Set.h>
#include <Core/IO/IO.h>
#include <Core/Memory/Memory.h>
#include <vk_mem_alloc.h>
#include <Engine/Importer/MeshImporter.h>
#include <Engine/Importer/TextureImporter.h>
#include <Rendering/Material.h>
#include <Rendering/RHI/RHI.h>
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
static const String         monkeyMeshPath        = "assets/VulkanGuide/monkey_smooth.obj";
static const ResourceHandle depthTextureHandle    = ResourceHandle(HashCalculate("__RHIDepthTexture"));
static const String         triangleMeshName      = "__Triangle";

static const u32 minMeshDataBufferSize = 65535;

struct alignas(16) MeshData
{
    glm::mat4 m;
};
} // namespace

const Vector<const char*> VulkanDevice::kDeviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

VulkanDevice::VulkanDevice()
{
}

VulkanDevice::~VulkanDevice()
{
    Cleanup();
}

void VulkanDevice::Initialize(SDL_Window* InWindowHandle, vk::Instance inInstance, vk::SurfaceKHR inSurface)
{
    instance = inInstance;
    surface  = inSurface;

    windowID = SDL_GetWindowID(InWindowHandle);

    /////// Initialize GPU

    Vector<vk::PhysicalDevice> devices = instance.enumeratePhysicalDevices();

    gpu = SelectPhysicalDevice(devices);

    _ASSERT(gpu);

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

    vk::PhysicalDeviceFeatures deviceFeatures {};

    vk::DeviceCreateInfo deviceCreateInfo {
        .queueCreateInfoCount    = static_cast<u32>(queueFamilies.size()),
        .pQueueCreateInfos       = queueFamilies.data(),
        .enabledExtensionCount   = static_cast<u32>(kDeviceExtensions.size()),
        .ppEnabledExtensionNames = kDeviceExtensions.data(),
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
                                                      .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
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

    // Upload Context

    vk::CommandPoolCreateInfo uploadCmdPoolCreateInfo {
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer, .queueFamilyIndex = Indices.Graphics.value()};

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

    ////// Render Pass

    //	Color Attachment

    // the renderpass will use this color attachment.
    vk::AttachmentDescription colorAttachmentDesc {
        .format = swapChainFormat.format,
        // 1 sample, we won't be doing MSAA
        .samples = vk::SampleCountFlagBits::e1,
        // we Clear when this attachment is loaded
        .loadOp = vk::AttachmentLoadOp::eClear,
        // we keep the attachment stored when the renderpass ends
        .storeOp = vk::AttachmentStoreOp::eStore,
        // we don't care about stencil
        .stencilLoadOp  = vk::AttachmentLoadOp::eDontCare,
        .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
        // we don't know or care about the starting layout of the attachment
        .initialLayout = vk::ImageLayout::eUndefined,
        // after the renderpass ends, the image has to be on a layout ready for display
        .finalLayout = vk::ImageLayout::ePresentSrcKHR,
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
        .flags = vk::AttachmentDescriptionFlags(0),
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
    vk::SubpassDependency colorDependency {
        .srcSubpass    = VK_SUBPASS_EXTERNAL,
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

    // we want to create the fence with the Create Signaled flag, so we can wait on it before using it on a GPU command (for the first frame)
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

        Vector<vk::DescriptorPoolSize> imguiPoolSizes = {
            {vk::DescriptorType::eSampler, 1000},
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
        ImGui_ImplVulkan_InitInfo imguiInitInfo {
            .Instance       = instance,
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

    LoadMeshes();

    {
        CreateTexture(defaultTexturePath);
        CreateTexture(vikingRoomTexturePath);
    }

    CreateMeshPipeline();

    CreateScene();

    isInitialized = true;
}

void VulkanDevice::Cleanup()
{
    if (isInitialized == false)
    {
        return;
    }

    device.waitIdle();

    CleanupSwapChain();

    for (auto& textureKvp : textures)
    {
        device.destroyImageView(textureKvp.second->imageView);
        allocator.destroyImage(textureKvp.second->image, textureKvp.second->allocation);
        delete textureKvp.second;
    }

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

    // if (surface != VK_NULL_HANDLE)
    //{
    //	vkDestroySurfaceKHR(instance, surface, nullptr/*allocator*/);
    //	surface = VK_NULL_HANDLE;
    // }

    if (device)
    {
        device.destroy();
    }

    // if (instance != VK_NULL_HANDLE)
    //{
    //	vkDestroyInstance(instance, nullptr/*allocator*/);
    //	instance = VK_NULL_HANDLE;
    // }

    isInitialized = false;
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
        glm::mat4 projection = glm::perspective(glm::radians(60.f), 16.f / 9.f, 0.1f, 200.f);
        projection[1][1] *= -1;

        GPUCameraData camera {};
        camera.projection      = projection;
        camera.view            = view;
        camera.view_projection = projection * view;

        CopyToGPU(cameraBuffer[currentFrame].allocation, &camera, sizeof(GPUCameraData));

        LightingUniforms lighting {};
        lighting.directional_lights[0].direction = glm::vec4(glm::normalize(glm::mat3(camera.view_projection) * glm::vec3(0.f, -1.f, 0.0f)), 0.0f);
        lighting.directional_lights[0].color     = glm::vec4(1.0f, 0.8f, 0.8f, 0.f);
        lighting.directional_lights[0].intensity = 0.25f;
        lighting.ambient_light.color             = glm::vec4(0.f, 0.2f, 1.f, 0.f);
        lighting.ambient_light.intensity         = 0.15f;
        lighting.num_directional_lights          = 1;
        lighting.num_point_lights                = 0;

        CopyToGPU(lightingBuffer[currentFrame].allocation, &lighting, sizeof(LightingUniforms));

        Vector<MeshData> meshData;
        meshData.reserve(renderables.size());

        for (const RenderObject& object : renderables)
        {
            static const auto identity = glm::mat4 {1.f};

            glm::mat4 translation = glm::translate(identity, object.location);
            glm::mat4 rotation    = glm::mat4_cast(object.rotation);
            glm::mat4 scale       = glm::scale(identity, object.scale);

            glm::mat4 transform = translation * rotation * scale;

            meshData.push_back(MeshData {.m = transform});
        }

        CopyToGPU(meshDataUBO[currentFrame].allocation, meshData.data(), meshData.size() * sizeof(MeshData));

        DrawObjects(commandBuffer, renderables.data(), renderables.size());

        // Enqueue ImGui commands to CmdBuffer
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
    }

    // finalize the render pass
    commandBuffer.endRenderPass();
    // finalize the command buffer (we can no longer add commands, but it can now be executed)
    commandBuffer.end();
}

void Zn::VulkanDevice::EndFrame()
{
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

    auto numFoundExtensions = std::count_if(
        availableExtensions.begin(), availableExtensions.end(),
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
    SwapChainDetails outDetails {
        .Capabilities = inGPU.getSurfaceCapabilitiesKHR(surface),
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
        {vk::DescriptorType::eUniformBuffer, 10},
        {vk::DescriptorType::eUniformBufferDynamic, 10},
        {vk::DescriptorType::eStorageBuffer, 10},
        {vk::DescriptorType::eCombinedImageSampler, 10}};

    vk::DescriptorPoolCreateInfo poolCreateInfo {
        .flags = (vk::DescriptorPoolCreateFlagBits) 0, .maxSets = 10, .poolSizeCount = ArrayLength(poolSizes), .pPoolSizes = ArrayData(poolSizes)};

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
            .stageFlags      = vk::ShaderStageFlagBits::eVertex,
        },
        // Lighting
        {
            .binding         = 1,
            .descriptorType  = vk::DescriptorType::eUniformBuffer,
            .descriptorCount = 1,
            .stageFlags      = vk::ShaderStageFlagBits::eFragment,
        },
        // Matrices
        {.binding = 2, .descriptorType = vk::DescriptorType::eUniformBufferDynamic, .descriptorCount = 1, .stageFlags = vk::ShaderStageFlagBits::eVertex}};

    vk::DescriptorSetLayoutCreateInfo globalSetCreateInfo {.bindingCount = ArrayLength(bindings), .pBindings = ArrayData(bindings)};

    globalDescriptorSetLayout = device.createDescriptorSetLayout(globalSetCreateInfo);

    destroyQueue.Enqueue(
        [=]()
        {
            device.destroyDescriptorSetLayout(globalDescriptorSetLayout);
        });

    // Single Texture Set

    vk::DescriptorSetLayoutBinding singleTextureSetBinding {
        .binding         = 0,
        .descriptorType  = vk::DescriptorType::eCombinedImageSampler,
        .descriptorCount = 1,
        .stageFlags      = vk::ShaderStageFlagBits::eFragment,
    };

    vk::DescriptorSetLayoutCreateInfo singleTextureSetCreateInfo {.bindingCount = 1, .pBindings = &singleTextureSetBinding};

    singleTextureSetLayout = device.createDescriptorSetLayout(singleTextureSetCreateInfo);

    destroyQueue.Enqueue(
        [=]()
        {
            device.destroyDescriptorSetLayout(singleTextureSetLayout);
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

        lightingBuffer[Index] = CreateBuffer(sizeof(LightingUniforms), vk::BufferUsageFlagBits::eUniformBuffer, vma::MemoryUsage::eCpuToGpu);

        destroyQueue.Enqueue(
            [=]()
            {
                DestroyBuffer(lightingBuffer[Index]);
            });

        // Matrices

        meshDataUBO[Index] = CreateBuffer(sizeof(MeshData) * minMeshDataBufferSize, vk::BufferUsageFlagBits::eUniformBuffer, vma::MemoryUsage::eCpuToGpu);

        destroyQueue.Enqueue(
            [=]()
            {
                DestroyBuffer(meshDataUBO[Index]);
            });

        vk::DescriptorSetAllocateInfo descriptorSetAllocInfo {};
        descriptorSetAllocInfo.descriptorPool     = descriptorPool;
        descriptorSetAllocInfo.descriptorSetCount = 1;
        descriptorSetAllocInfo.pSetLayouts        = &globalDescriptorSetLayout;

        globalDescriptorSets[Index] = device.allocateDescriptorSets(descriptorSetAllocInfo)[0];

        vk::DescriptorBufferInfo bufferInfo[] = {
            vk::DescriptorBufferInfo {cameraBuffer[Index].data, 0, sizeof(GPUCameraData)},
            vk::DescriptorBufferInfo {lightingBuffer[Index].data, 0, sizeof(LightingUniforms)},
            vk::DescriptorBufferInfo {meshDataUBO[Index].data, 0, sizeof(MeshData)}};

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
            vk::WriteDescriptorSet {
                .dstSet          = globalDescriptorSets[Index],
                .dstBinding      = 2,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType  = vk::DescriptorType::eUniformBufferDynamic,
                .pBufferInfo     = &bufferInfo[2],
            }};

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

    const bool useMailboxPresentMode = std::any_of(
        SwapChainDetails.PresentModes.begin(), SwapChainDetails.PresentModes.end(),
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

    Width = std::clamp(
        Width, static_cast<int32>(SwapChainDetails.Capabilities.minImageExtent.width), static_cast<int32>(SwapChainDetails.Capabilities.maxImageExtent.width));
    Height = std::clamp(
        Height, static_cast<int32>(SwapChainDetails.Capabilities.minImageExtent.height),
        static_cast<int32>(SwapChainDetails.Capabilities.maxImageExtent.height));

    uint32 ImageCount = SwapChainDetails.Capabilities.minImageCount + 1;
    if (SwapChainDetails.Capabilities.maxImageCount > 0)
    {
        ImageCount = std::min(ImageCount, SwapChainDetails.Capabilities.maxImageCount);
    }

    swapChainFormat = surfaceFormat;

    swapChainExtent = vk::Extent2D(Width, Height);

    vk::SwapchainCreateInfoKHR swapChainCreateInfo {
        .surface               = surface,
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
        vk::ImageViewCreateInfo imageViewCreateInfo {
            .image    = swapChainImages[Index],
            .viewType = vk::ImageViewType::e2D,
            .format   = swapChainFormat.format,
            // RGBA
            .components = {vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity},
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

    if (auto result = textures.insert(
            {depthTextureHandle,
             new RHITexture {
                 .width  = static_cast<i32>(swapChainExtent.width),
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
        vk::ImageCreateInfo depthImageCreateInfo = MakeImageCreateInfo(depthTexture->format, vk::ImageUsageFlagBits::eDepthStencilAttachment, depthImageExtent);

        //	Allocate from GPU memory.

        vma::AllocationCreateInfo depthImageAllocInfo {
            //	VMA_MEMORY_USAGE_GPU_ONLY to make sure that the image is allocated on fast VRAM.
            .usage = vma::MemoryUsage::eGpuOnly,
            //	To make absolutely sure that VMA really allocates the image into VRAM, we give it VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT on required flags.
            //	This forces VMA library to allocate the image on VRAM no matter what. (The Memory Usage part is more like a hint)
            .requiredFlags = vk::MemoryPropertyFlags(vk::MemoryPropertyFlagBits::eDeviceLocal),
        };

        ZN_VK_CHECK(allocator.createImage(&depthImageCreateInfo, &depthImageAllocInfo, &depthTexture->image, &depthTexture->allocation, nullptr));

        //	VK_IMAGE_ASPECT_DEPTH_BIT lets the vulkan driver know that this will be a depth image used for z-testing.
        vk::ImageViewCreateInfo depthImageViewCreateInfo = MakeImageViewCreateInfo(depthTexture->format, depthTexture->image, vk::ImageAspectFlagBits::eDepth);

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

RHIBuffer Zn::VulkanDevice::CreateBuffer(size_t size, vk::BufferUsageFlags usage, vma::MemoryUsage memoryUsage) const
{
    vk::BufferCreateInfo createInfo {.size = size, .usage = usage};

    vma::AllocationCreateInfo allocationInfo {
        .usage         = memoryUsage,
        .requiredFlags = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
    };

    return RHIBuffer(allocator.createBuffer(createInfo, allocationInfo));
}

void Zn::VulkanDevice::DestroyBuffer(RHIBuffer buffer) const
{
    allocator.destroyBuffer(buffer.data, buffer.allocation);
}

void Zn::VulkanDevice::CopyToGPU(vma::Allocation allocation, void* src, size_t size) const
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
    RHIMesh*  lastMesh     = nullptr;
    Material* lastMaterial = nullptr;

    for (u32 index = 0; index < count; ++index)
    {
        RenderObject& object = first[index];

        static const auto identity = glm::mat4 {1.f};

        glm::mat4 translation = glm::translate(identity, object.location);
        glm::mat4 rotation    = glm::mat4_cast(object.rotation);
        glm::mat4 scale       = glm::scale(identity, object.scale);

        glm::mat4 transform = translation * rotation * scale;

        // only bind the pipeline if it doesn't match what's already bound
        if (object.material != lastMaterial)
        {
            commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, object.material->pipeline);

            SetViewport(commandBuffer);

            lastMaterial = object.material;

            if (object.material->textureSet)
            {
                commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, object.material->layout, 1, object.material->textureSet, {});
            }
        }

        // TODO: Rebinding global for every mesh.
        // I think we can make a different set that is refreshed at this rate rather than rebinding 3 sets all the time.
        u32 offset = sizeof(MeshData) * index;

        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, object.material->layout, 0, globalDescriptorSets[currentFrame], {offset});

        MeshPushConstants constants {.RenderMatrix = transform};

        commandBuffer.pushConstants(object.material->layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(MeshPushConstants), &constants);

        // only bind the mesh if it's different from what's already bound

        if (object.mesh != lastMesh)
        {
            // bind the mesh v-buffer with offset 0
            vk::DeviceSize offset = 0;
            commandBuffer.bindVertexBuffers(0, 1, &object.mesh->vertexBuffer.data, &offset);

            if (object.mesh->indexBuffer.data)
            {
                commandBuffer.bindIndexBuffer(object.mesh->indexBuffer.data, 0, vk::IndexType::eUint32);
            }
            lastMesh = object.mesh;
        }

        // Draw

        if (object.mesh->indexBuffer.data)
        {
            commandBuffer.drawIndexed(object.mesh->indices.size(), 1, 0, 0, 0);
        }
        else
        {
            commandBuffer.draw(object.mesh->vertices.size(), 1, 0, 0);
        }
    }
}

void Zn::VulkanDevice::CreateScene()
{
    RenderObject viking_room {
        .mesh     = GetMesh(vikingRoomMeshPath),
        .material = VulkanMaterialManager::Get().GetMaterial("default"),
        .location = glm::vec3(0.f),
        .rotation = glm::quat(),
        .scale    = glm::vec3(1.f),
    };

    // Sampler

    vk::DescriptorSetAllocateInfo singleTextureAllocateInfo {
        .descriptorPool     = descriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts        = &singleTextureSetLayout,
    };

    viking_room.material->textureSet = device.allocateDescriptorSets(singleTextureAllocateInfo)[0];

    const vk::Filter             samplerFilters     = vk::Filter::eNearest;
    const vk::SamplerAddressMode samplerAddressMode = vk::SamplerAddressMode::eRepeat;

    vk::SamplerCreateInfo samplerCreateInfo {
        .magFilter    = samplerFilters,
        .minFilter    = samplerFilters,
        .addressModeU = samplerAddressMode,
        .addressModeV = samplerAddressMode,
        .addressModeW = samplerAddressMode,
    };

    vk::Sampler sampler = device.createSampler(samplerCreateInfo);
    destroyQueue.Enqueue(
        [=]()
        {
            device.destroySampler(sampler);
        });

    viking_room.material->textureSamplers["default"] = sampler;

    // write to the descriptor set so that it points to our empire_diffuse texture
    vk::DescriptorImageInfo imageBufferInfo;
    imageBufferInfo.sampler     = sampler;
    imageBufferInfo.imageView   = textures[HashCalculate(vikingRoomTexturePath)]->imageView;
    imageBufferInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

    device.updateDescriptorSets(
        {vk::WriteDescriptorSet {
            .dstSet          = viking_room.material->textureSet,
            .dstBinding      = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType  = vk::DescriptorType::eCombinedImageSampler,
            .pImageInfo      = &imageBufferInfo}},
        {});

    renderables.push_back(viking_room);

    RenderObject monkey {
        .mesh     = GetMesh(monkeyMeshPath),
        .material = VulkanMaterialManager::Get().GetMaterial("default"),
        .location = glm::vec3(0.f, 0.f, 10.f),
        .rotation = glm::quat(),
        .scale    = glm::vec3(1.f)};

    renderables.push_back(monkey);

    RHIMesh*  triangleMesh    = GetMesh(triangleMeshName);
    Material* defaultMaterial = VulkanMaterialManager::Get().GetMaterial("default");

    for (int32 x = -20; x <= 20; ++x)
    {
        for (int32 y = -20; y <= 20; ++y)
        {
            RenderObject triangle {
                .mesh     = triangleMesh,
                .material = defaultMaterial,
                .location = glm::vec3(x, 0, y),
                .rotation = glm::quat(glm::vec3(glm::radians(25.f), 0.f, 0.f)),
                .scale    = glm::vec3(0.2f)};

            renderables.push_back(triangle);
        }
    }
}

void Zn::VulkanDevice::LoadMeshes()
{
    RHIMesh* triangle = new RHIMesh();

    triangle->vertices.resize(3);

    // vertex positions
    triangle->vertices[0].position = {1.f, 1.f, 0.f};
    triangle->vertices[1].position = {-1.f, 1.f, 0.0f};
    triangle->vertices[2].position = {0.f, -0.5f, 0.0f};

    triangle->vertices[0].color = {1.0f, 0.f, 0.f};
    triangle->vertices[1].color = {0.0f, 1.0f, 0.0f};
    triangle->vertices[2].color = {0.0f, 0.0f, 1.0f};

    triangle->vertices[0].normal = glm::vec3(1.0f);
    triangle->vertices[1].normal = glm::vec3(1.0f);
    triangle->vertices[2].normal = glm::vec3(1.0f);

    triangle->vertices[0].uv = glm::vec2(1.0f);
    triangle->vertices[1].uv = glm::vec2(0.0, 1.f);
    triangle->vertices[2].uv = glm::vec2(0.0f);

    // we don't care about the vertex normals

    triangle->vertexBuffer = CreateRHIBuffer(
        triangle->vertices.data(), triangle->vertices.size() * sizeof(RHIVertex), vk::BufferUsageFlagBits::eVertexBuffer, vma::MemoryUsage::eGpuOnly);

    meshes.insert({ResourceHandle(HashCalculate(triangleMeshName)), triangle});

    RHIMesh* monkey = new RHIMesh();

    if (!MeshImporter::Import(IO::GetAbsolutePath(monkeyMeshPath), *monkey))
    {
        delete monkey;
        monkey = nullptr;
    }

    RHIMesh* vikingRoom = new RHIMesh();

    if (!MeshImporter::Import(IO::GetAbsolutePath(vikingRoomMeshPath), *vikingRoom))
    {
        delete vikingRoom;
        vikingRoom = nullptr;
    }

    if (monkey)
    {
        monkey->vertexBuffer = CreateRHIBuffer(
            monkey->vertices.data(), monkey->vertices.size() * sizeof(RHIVertex), vk::BufferUsageFlagBits::eVertexBuffer, vma::MemoryUsage::eGpuOnly);

        monkey->indexBuffer =
            CreateRHIBuffer(monkey->indices.data(), monkey->indices.size() * sizeof(i32), vk::BufferUsageFlagBits::eIndexBuffer, vma::MemoryUsage::eGpuOnly);

        meshes.insert({ResourceHandle(HashCalculate(monkeyMeshPath)), monkey});
    }

    if (vikingRoom)
    {
        vikingRoom->vertexBuffer = CreateRHIBuffer(
            vikingRoom->vertices.data(), vikingRoom->vertices.size() * sizeof(RHIVertex), vk::BufferUsageFlagBits::eVertexBuffer, vma::MemoryUsage::eGpuOnly);

        vikingRoom->indexBuffer = CreateRHIBuffer(
            vikingRoom->indices.data(), vikingRoom->indices.size() * sizeof(i32), vk::BufferUsageFlagBits::eIndexBuffer, vma::MemoryUsage::eGpuOnly);

        meshes.insert({ResourceHandle(HashCalculate(vikingRoomMeshPath)), vikingRoom});
    }
}

RHIBuffer Zn::VulkanDevice::CreateRHIBuffer(void* data, sizet size, vk::BufferUsageFlags bufferUsage, vma::MemoryUsage memoryUsage) const
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

    const bool vertex_success   = IO::ReadBinaryFile("shaders/vertex.vert.spv", vertex_shader_data);
    const bool fragment_success = IO::ReadBinaryFile("shaders/fragment.frag.spv", fragment_shader_data);

    if (vertex_success && fragment_success)
    {
        Material* material = VulkanMaterialManager::Get().CreateMaterial("default");

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

        // Mesh pipeline with push constants

        vk::DescriptorSetLayout layouts[2] = {globalDescriptorSetLayout, singleTextureSetLayout};

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

        material->pipeline =
            VulkanPipeline::NewVkPipeline(device, renderPass, material->vertexShader, material->fragmentShader, swapChainExtent, material->layout);

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

        stagingBuffer = CreateBuffer(sourceTexture->size, vk::BufferUsageFlagBits::eTransferSrc, vma::MemoryUsage::eCpuOnly);
        CopyToGPU(stagingBuffer.allocation, sourceTexture->data, sourceTexture->size);
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
            TransitionImageLayout(cmd, texture->image, vk::Format::eR8G8B8A8Srgb, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
            CopyBufferToImage(cmd, stagingBuffer.data, texture->image, width, height);
            TransitionImageLayout(
                cmd, texture->image, vk::Format::eR8G8B8A8Srgb, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
        });

    DestroyBuffer(stagingBuffer);

    vk::ImageViewCreateInfo imageViewInfo {
        .image            = texture->image,
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

    return texture;
}

RHITexture* Zn::VulkanDevice::CreateRHITexture(i32 width, i32 height, vk::Format format) const
{
    vk::ImageCreateInfo createInfo = MakeImageCreateInfo(
        format, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, vk::Extent3D {static_cast<u32>(width), static_cast<u32>(height), 1});

    createInfo.initialLayout = vk::ImageLayout::eUndefined;
    createInfo.sharingMode   = vk::SharingMode::eExclusive;

    //	Allocate from GPU memory.

    vma::AllocationCreateInfo allocationInfo {
        //	VMA_MEMORY_USAGE_GPU_ONLY to make sure that the image is allocated on fast VRAM.
        .usage = vma::MemoryUsage::eGpuOnly,
        //	To make absolutely sure that VMA really allocates the image into VRAM, we give it VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT on required flags.
        //	This forces VMA library to allocate the image on VRAM no matter what. (The Memory Usage part is more like a hint)
        .requiredFlags = vk::MemoryPropertyFlags(vk::MemoryPropertyFlagBits::eDeviceLocal)};

    RHITexture* texture = new RHITexture {.width = width, .height = height};

    ZN_VK_CHECK(allocator.createImage(&createInfo, &allocationInfo, &texture->image, &texture->allocation, nullptr));

    return texture;
}

void Zn::VulkanDevice::TransitionImageLayout(vk::CommandBuffer cmd, vk::Image img, vk::Format fmt, vk::ImageLayout prevLayout, vk::ImageLayout newLayout) const
{
    vk::ImageMemoryBarrier barrier {
        .oldLayout = prevLayout,
        .newLayout = newLayout,
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

    function(cmd);

    cmd.end();

    vk::SubmitInfo submitInfo {.commandBufferCount = 1, .pCommandBuffers = &cmd};

    // .fence will now block until the graphic commands finish execution
    graphicsQueue.submit(submitInfo, uploadContext.fence);

    ZN_VK_CHECK(device.waitForFences({uploadContext.fence}, true, kWaitTimeOneSecond));
    device.resetFences({uploadContext.fence});

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
    //	.samples = controls the MSAA behavior of the texture. This only makes sense for render targets, such as depth images and images you are rendering to.
    //					TODO: We won’t be doing MSAA in this tutorial, so samples will be kept at 1 sample.
    //  .tiling = if you use VK_IMAGE_TILING_OPTIMAL, it won’t be possible to read the data from CPU or to write it without changing its tiling first
    //					(it’s possible to change the tiling of a texture at any point, but this can be a costly operation).
    //					The other tiling we can care about is VK_IMAGE_TILING_LINEAR, which will store the image as a 2d array of pixels.
    //	.usage = controls how the GPU handles the image memory.

    return {
        .imageType   = vk::ImageType::e2D,
        .format      = format,
        .extent      = extent,
        .mipLevels   = 1,
        .arrayLayers = 1,
        .samples     = vk::SampleCountFlagBits::e1,
        .tiling      = vk::ImageTiling::eOptimal,
        .usage       = usageFlags};
}
vk::ImageViewCreateInfo Zn::VulkanDevice::MakeImageViewCreateInfo(vk::Format format, vk::Image image, vk::ImageAspectFlagBits aspectFlags) const
{
    return {
        .image = image,
        //	While imageType held the dimensionality of the texture, viewType has a lot more options, like VK_IMAGE_VIEW_TYPE_CUBE for cubemaps.
        .viewType = vk::ImageViewType::e2D,
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
    vk::BufferImageCopy region {
        .bufferOffset      = 0,
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
