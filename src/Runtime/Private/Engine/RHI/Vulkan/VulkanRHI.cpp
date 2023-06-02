#include <Engine/RHI/RHIDevice.h>
#include <Engine/RHI/Vulkan/Vulkan.h>
#include <Engine/RHI/Vulkan/VulkanPlatform.h>
#include <Engine/RHI/Vulkan/VulkanGPU.h>
#include <Core/CoreAssert.h>

using namespace Zn;

struct QueueFamilyIndices
{
    std::optional<uint32> graphics;
    std::optional<uint32> present;
};

namespace
{
const Vector<cstring> GValidationExtensions = {VK_EXT_DEBUG_UTILS_EXTENSION_NAME};
const Vector<cstring> GValidationLayers     = {"VK_LAYER_KHRONOS_validation"};

vk::Instance         GVkInstance   = nullptr;
vk::SurfaceKHR       GVkSurfaceKHR = nullptr;
VulkanGPU*           GVulkanGPU    = nullptr;
vk::Device           GVkDevice     = nullptr;
vma::Allocator       GVkAllocator  = nullptr;
vk::SurfaceFormatKHR GVkSwapChainFormat;
vk::Extent2D         GVkSwapChainExtent;
vk::SwapchainKHR     GVkSwapChain = nullptr;
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
}

void RHIDevice::CleanupSwapChain()
{
    GVkDevice.destroySwapchainKHR(GVkSwapChain);

    GVkSwapChain = nullptr;
}
