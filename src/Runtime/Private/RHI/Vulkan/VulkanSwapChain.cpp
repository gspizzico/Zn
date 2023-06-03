#include <RHI/Vulkan/VulkanSwapChain.h>
#include <RHI/Vulkan/VulkanPlatform.h>
#include <RHI/Vulkan/VulkanContext.h>

void Zn::VulkanSwapChain::Create()
{
    check(!swapChain && imageCount == 0);

    VulkanContext& vkContext = VulkanContext::Get();

    vk::PhysicalDevice gpu     = vkContext.gpu.gpu;
    vk::Device         device  = vkContext.device;
    vk::SurfaceKHR     surface = vkContext.surface;

    surfaceFormat = {vk::Format::eUndefined, vk::ColorSpaceKHR::eSrgbNonlinear};

    auto gpuSurfaceFormats = gpu.getSurfaceFormatsKHR(vkContext.surface);

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

    // Always guaranteed to be available.
    presentMode = vk::PresentModeKHR::eFifo;

    auto gpuPresentModes = gpu.getSurfacePresentModesKHR(surface);

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

    vk::SurfaceCapabilitiesKHR capabilities = gpu.getSurfaceCapabilitiesKHR(surface);

    width  = std::clamp(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    height = std::clamp(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0)
    {
        imageCount = std::min(imageCount, capabilities.maxImageCount);
    }

    check(imageCount <= kMaxImageCount);

    extent = vk::Extent2D {
        .width  = width,
        .height = height,
    };

    vk::SwapchainCreateInfoKHR swapChainCreateInfo {.surface               = surface,
                                                    .minImageCount         = imageCount,
                                                    .imageFormat           = surfaceFormat.format,
                                                    .imageColorSpace       = surfaceFormat.colorSpace,
                                                    .imageExtent           = extent,
                                                    .imageArrayLayers      = 1,
                                                    .imageUsage            = vk::ImageUsageFlagBits::eColorAttachment,
                                                    .imageSharingMode      = vk::SharingMode::eExclusive,
                                                    .queueFamilyIndexCount = 0,
                                                    .pQueueFamilyIndices   = nullptr,
                                                    .preTransform          = capabilities.currentTransform,
                                                    .compositeAlpha        = vk::CompositeAlphaFlagBitsKHR::eOpaque,
                                                    .presentMode           = presentMode,
                                                    .clipped               = true};

    uint32 queueFamilies[] = {vkContext.gpu.graphicsQueue, vkContext.gpu.presentQueue};

    if (vkContext.gpu.graphicsQueue != vkContext.gpu.presentQueue)
    {
        swapChainCreateInfo.imageSharingMode = vk::SharingMode::eConcurrent;
        swapChainCreateInfo.setQueueFamilyIndices(queueFamilies);
    }

    swapChain = device.createSwapchainKHR(swapChainCreateInfo);

    Vector<vk::Image> vImages = device.getSwapchainImagesKHR(swapChain);

    imageCount = static_cast<uint32>(vImages.size());
    check(imageCount <= kMaxImageCount);

    for (uint32 index = 0; index < imageCount; ++index)
    {
        images[index] = vImages[index];

        vk::ImageViewCreateInfo imageViewCreateInfo {.image            = images[index],
                                                     .viewType         = vk::ImageViewType::e2D,
                                                     .format           = surfaceFormat.format,
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

        imageViews[index] = device.createImageView(imageViewCreateInfo);
    }
}

void Zn::VulkanSwapChain::Destroy()
{
    VulkanContext& vkContext = VulkanContext::Get();

    for (uint32 index = 0; index < imageCount; ++index)
    {
        vkContext.device.destroyImageView(imageViews[index]);
    }

    vkContext.device.destroySwapchainKHR(swapChain);

    *this = VulkanSwapChain();
}
