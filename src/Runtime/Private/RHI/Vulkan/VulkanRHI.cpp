#include <RHI/RHIDevice.h>
#include <RHI/Vulkan/Vulkan.h>
#include <RHI/Vulkan/VulkanPlatform.h>
#include <RHI/Vulkan/VulkanGPU.h>
#include <Core/CoreAssert.h>
#include <RHI/RHIResource.h>
#include <RHI/RHITexture.h>
#include <RHI/Vulkan/VulkanTexture.h>
#include <RHI/RHIBuffer.h>
#include <RHI/Vulkan/VulkanBuffer.h>
#include <RHI/Vulkan/VulkanResource.h>
#include <Core/Misc/Hash.h>

#define MAKE_RESOURCE_HANDLE(name) ResourceHandle(HashCalculate(name))

using namespace Zn;

struct QueueFamilyIndices
{
    std::optional<uint32> graphics;
    std::optional<uint32> present;
};

namespace
{
static const cstring       GDepthTextureName = "__DepthTexture";
static const TextureHandle GDepthTextureHandle(MAKE_RESOURCE_HANDLE(GDepthTextureName));

using TTextureResource = TResource<RHITexture, VulkanTexture>;
using TBufferResource  = TResource<RHIBuffer, VulkanBuffer>;

const Vector<cstring> GValidationExtensions = {VK_EXT_DEBUG_UTILS_EXTENSION_NAME};
const Vector<cstring> GValidationLayers     = {"VK_LAYER_KHRONOS_validation"};

vk::Instance                          GVkInstance   = nullptr;
vk::SurfaceKHR                        GVkSurfaceKHR = nullptr;
VulkanGPU*                            GVulkanGPU    = nullptr;
vk::Device                            GVkDevice     = nullptr;
vma::Allocator                        GVkAllocator  = nullptr;
vk::SurfaceFormatKHR                  GVkSwapChainFormat;
vk::Extent2D                          GVkSwapChainExtent;
vk::SwapchainKHR                      GVkSwapChain = nullptr;
Vector<vk::Image>                     GVkSwapChainImages;
Vector<vk::ImageView>                 GVkSwapChainImageViews;
vk::RenderPass                        GVkRenderPass;
Vector<vk::Framebuffer>               GVkFrameBuffers;
Map<ResourceHandle, TTextureResource> GTextures;

TTextureResource CreateRHITexture(const RHITextureDescriptor& descriptor_)
{
    vk::ImageCreateInfo createInfo {
        .imageType   = vk::ImageType::e2D,
        .format      = TranslateRHIFormat(descriptor_.format),
        .extent      = vk::Extent3D {descriptor_.width, descriptor_.height, 1},
        .mipLevels   = descriptor_.numMips,
        .arrayLayers = 1,
        .samples     = vk::SampleCountFlagBits::e1,
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

    ZN_VK_CHECK(GVkAllocator.createImage(&createInfo, &allocation, &texture.payload.image, &texture.payload.memory, nullptr));

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

    check(GVkAllocator);

    auto [buffer, allocation] = GVkAllocator.createBuffer(createInfo, allocationInfo);

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
    void* dst = GVkAllocator.mapMemory(allocation_);

    memcpy(dst, src_, size_);

    GVkAllocator.unmapMemory(allocation_);
}
} // namespace

#if ZN_VK_VALIDATION_LAYERS
namespace Zn::VulkanValidation
{
vk::DebugUtilsMessengerEXT GVkDebugMessenger {};

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

void InitializeDebugMessenger(vk::Instance instance_)
{
    check(!GVkDebugMessenger);

    GVkDebugMessenger = instance_.createDebugUtilsMessengerEXT(GetDebugMessengerCreateInfo());
}

void DeinitializeDebugMessenger(vk::Instance instance_)
{
    if (GVkDebugMessenger)
    {
        instance_.destroyDebugUtilsMessengerEXT(GVkDebugMessenger);
        GVkDebugMessenger = nullptr;
    }
}

bool SupportsValidationLayers()
{
    Vector<vk::LayerProperties> availableLayers = vk::enumerateInstanceLayerProperties();

    return std::any_of(availableLayers.begin(),
                       availableLayers.end(),
                       [](const vk::LayerProperties& it)
                       {
                           for (const auto& layerName : GValidationLayers)
                           {
                               if (strcmp(it.layerName, layerName) == 0)
                               {
                                   return true;
                               }
                           }

                           return false;
                       });
}
} // namespace Zn::VulkanValidation
#endif

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
    // Request debug utils extension if validation layers are enabled.
    instanceExtensions.insert(instanceExtensions.end(), GValidationExtensions.begin(), GValidationExtensions.end());

    if (VulkanValidation::SupportsValidationLayers())
    {
        instanceCreateInfo.setPEnabledLayerNames(GValidationLayers);
    }
#endif

    instanceCreateInfo.setPEnabledExtensionNames(instanceExtensions);

    GVkInstance = vk::createInstance(instanceCreateInfo);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(GVkInstance);

#if ZN_VK_VALIDATION_LAYERS
    VulkanValidation::InitializeDebugMessenger(GVkInstance);
#endif

    GVkSurfaceKHR = PlatformVulkan::CreateSurface(GVkInstance);

    GVulkanGPU = new VulkanGPU(GVkInstance, GVkSurfaceKHR);

    // Logical Device

    GVkDevice = GVulkanGPU->CreateDevice();

    VULKAN_HPP_DEFAULT_DISPATCHER.init(GVkDevice);

    // Vma Allocator
    vma::AllocatorCreateInfo allocatorCreateInfo {.physicalDevice = GVulkanGPU->gpu, .device = GVkDevice, .instance = GVkInstance};

    GVkAllocator = vma::createAllocator(allocatorCreateInfo);

    CreateSwapChain();
}

RHIDevice::~RHIDevice()
{
    if (!GVkInstance)
    {
        return;
    }

    if (GVkDevice)
    {
        GVkDevice.waitIdle();
    }

    CleanupSwapChain();

    for (auto& textureResource : GTextures)
    {
        VulkanTexture& texture = textureResource.second.payload;

        GVkDevice.destroyImageView(texture.imageView);
        GVkAllocator.destroyImage(texture.image, texture.memory);
    }

    GTextures.clear();

    if (GVkAllocator)
    {
        GVkAllocator.destroy();
        GVkAllocator = nullptr;
    }

    if (GVkSurfaceKHR)
    {
        GVkInstance.destroySurfaceKHR(GVkSurfaceKHR);
        GVkSurfaceKHR = nullptr;
    }

    if (GVkDevice)
    {
        GVkDevice.destroy();
        GVkDevice = nullptr;
    }

    delete GVulkanGPU;
    GVulkanGPU = nullptr;

#if ZN_VK_VALIDATION_LAYERS
    VulkanValidation::DeinitializeDebugMessenger(GVkInstance);
#endif

    GVkInstance.destroy();
    GVkInstance = nullptr;
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

    TextureHandle resourceHandle(MAKE_RESOURCE_HANDLE(descriptor_.id));

    GTextures.try_emplace(resourceHandle, std::move(texture));

    return resourceHandle;
}

void Zn::RHIDevice::CreateSwapChain()
{
    vk::SurfaceFormatKHR surfaceFormat = {vk::Format::eUndefined, vk::ColorSpaceKHR::eSrgbNonlinear};

    auto gpuSurfaceFormats = GVulkanGPU->gpu.getSurfaceFormatsKHR(GVkSurfaceKHR);

    for (const vk::SurfaceFormatKHR& format : gpuSurfaceFormats)
    {
        if (format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
        {
            surfaceFormat = format;
            break;
        }
    }

    if (surfaceFormat.format == vk::Format::eUndefined)
    {
        ZN_LOG(LogVulkan, ELogVerbosity::Warning, "Unable to find requrested SurfaceFormatKHR. Fallback to first available.");

        surfaceFormat = gpuSurfaceFormats[0];
    }

    GVkSwapChainFormat = surfaceFormat;

    // Always guaranteed to be available.
    vk::PresentModeKHR presentMode = vk::PresentModeKHR::eFifo;

    auto gpuPresentModes = GVulkanGPU->gpu.getSurfacePresentModesKHR(GVkSurfaceKHR);

    const bool useMailboxPresentMode = std::any_of(gpuPresentModes.begin(),
                                                   gpuPresentModes.end(),
                                                   [](vk::PresentModeKHR presentMode_)
                                                   {
                                                       // 'Triple Buffering'
                                                       return presentMode_ == vk::PresentModeKHR::eMailbox;
                                                   });

    if (useMailboxPresentMode)
    {
        presentMode = vk::PresentModeKHR::eMailbox;
    }

    uint32 width, height;
    PlatformVulkan::GetSurfaceDrawableSize(width, height);

    vk::SurfaceCapabilitiesKHR capabilities = GVulkanGPU->gpu.getSurfaceCapabilitiesKHR(GVkSurfaceKHR);

    width  = std::clamp(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    height = std::clamp(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    uint32 imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0)
    {
        imageCount = std::min(imageCount, capabilities.maxImageCount);
    }

    GVkSwapChainExtent = vk::Extent2D {
        .width  = width,
        .height = height,
    };

    vk::SwapchainCreateInfoKHR swapChainCreateInfo {.surface               = GVkSurfaceKHR,
                                                    .minImageCount         = imageCount,
                                                    .imageFormat           = GVkSwapChainFormat.format,
                                                    .imageColorSpace       = GVkSwapChainFormat.colorSpace,
                                                    .imageExtent           = GVkSwapChainExtent,
                                                    .imageArrayLayers      = 1,
                                                    .imageUsage            = vk::ImageUsageFlagBits::eColorAttachment,
                                                    .imageSharingMode      = vk::SharingMode::eExclusive,
                                                    .queueFamilyIndexCount = 0,
                                                    .pQueueFamilyIndices   = nullptr,
                                                    .preTransform          = capabilities.currentTransform,
                                                    .compositeAlpha        = vk::CompositeAlphaFlagBitsKHR::eOpaque,
                                                    .presentMode           = presentMode,
                                                    .clipped               = true};

    if (GVulkanGPU->graphicsQueue != GVulkanGPU->presentQueue)
    {
        uint32 queueFamilies[]               = {GVulkanGPU->graphicsQueue, GVulkanGPU->presentQueue};
        swapChainCreateInfo.imageSharingMode = vk::SharingMode::eConcurrent;
        swapChainCreateInfo.setQueueFamilyIndices(queueFamilies);
    }

    GVkSwapChain = GVkDevice.createSwapchainKHR(swapChainCreateInfo);

    GVkSwapChainImages = GVkDevice.getSwapchainImagesKHR(GVkSwapChain);

    GVkSwapChainImageViews.resize(GVkSwapChainImages.size());

    for (auto index = 0; index < GVkSwapChainImages.size(); ++index)
    {
        vk::ImageViewCreateInfo imageViewCreateInfo {.image            = GVkSwapChainImages[index],
                                                     .viewType         = vk::ImageViewType::e2D,
                                                     .format           = GVkSwapChainFormat.format,
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

        GVkSwapChainImageViews[index] = GVkDevice.createImageView(imageViewCreateInfo);
    }

    TTextureResource depthTexture = CreateRHITexture(RHITextureDescriptor {
        .width       = GVkSwapChainExtent.width,
        .height      = GVkSwapChainExtent.height,
        .numMips     = 1,
        .numChannels = 1,
        .channelSize = sizeof(float),
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

    depthTexture.payload.imageView = GVkDevice.createImageView(depthTextureViewCreateInfo);

    GTextures.emplace(GDepthTextureHandle, std::move(depthTexture));
}

void RHIDevice::CleanupSwapChain()
{
    for (auto index = 0; index < GVkSwapChainImageViews.size(); ++index)
    {
        GVkDevice.destroyImageView(GVkSwapChainImageViews[index]);
    }

    if (auto it = GTextures.find(GDepthTextureHandle); it != GTextures.end())
    {
        TTextureResource& depthTexture = it->second;

        GVkDevice.destroyImageView(depthTexture.payload.imageView);
        GVkAllocator.destroyImage(depthTexture.payload.image, depthTexture.payload.memory);

        GTextures.erase(it);
    }

    GVkDevice.destroySwapchainKHR(GVkSwapChain);

    GVkSwapChain = nullptr;
}

void RHIDevice::CreateFrameBuffers()
{
    vk::FramebufferCreateInfo frameBufferCreate {
        .renderPass = GVkRenderPass,
        .width      = GVkSwapChainExtent.width,
        .height     = GVkSwapChainExtent.height,
        .layers     = 1,
    };

    GVkFrameBuffers = Vector<vk::Framebuffer>(GVkSwapChainImages.size());

    TTextureResource& depthTexture = GTextures[GDepthTextureHandle];

    for (auto index = 0; index < GVkSwapChainImages.size(); ++index)
    {
        vk::ImageView attachments[2] = {GVkSwapChainImageViews[index], depthTexture.payload.imageView};

        frameBufferCreate.setAttachments(attachments);

        GVkFrameBuffers[index] = GVkDevice.createFramebuffer(frameBufferCreate);
    }
}

void RHIDevice::CleanupFrameBuffers()
{
    for (vk::Framebuffer& frameBuffer : GVkFrameBuffers)
    {
        GVkDevice.destroyFramebuffer(frameBuffer);
    }
}
