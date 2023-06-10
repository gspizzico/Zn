#include <RHI/RHIDevice.h>
#include <RHI/Vulkan/Vulkan.h>
#include <RHI/Vulkan/VulkanDebug.h>
#include <RHI/Vulkan/VulkanPlatform.h>
#include <RHI/Vulkan/VulkanGPU.h>
#include <RHI/Vulkan/VulkanContext.h>
#include <RHI/RHIResourceBuffer.h>
#include <Core/CoreAssert.h>
#include <RHI/RHIResource.h>
#include <RHI/RHITexture.h>
#include <RHI/Vulkan/VulkanTexture.h>
#include <RHI/RHIBuffer.h>
#include <RHI/Vulkan/VulkanBuffer.h>
#include <RHI/Vulkan/VulkanResource.h>
#include <RHI/Vulkan/VulkanSwapChain.h>
#include <Core/Misc/Hash.h>

#define MAKE_RESOURCE_HANDLE(name) ResourceHandle(HashCalculate(name))

using namespace Zn;

namespace
{
static const cstring GDepthTextureName = "__DepthTexture";
static TextureHandle GDepthTextureHandle;

using TTextureResource = TResource<RHITexture, VulkanTexture>;
using TBufferResource  = TResource<RHIBuffer, VulkanBuffer>;

VulkanSwapChain                                                 GSwapChain;
vk::RenderPass                                                  GVkRenderPass;
Vector<vk::Framebuffer>                                         GVkFrameBuffers;
RHIResourceBuffer<TTextureResource, TextureHandle, u16_max + 1> GTextures;

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

    // Vma Allocator
    vma::AllocatorCreateInfo allocatorCreateInfo {
        .physicalDevice = vkContext.gpu.gpu, .device = vkContext.device, .instance = vkContext.instance};

    vkContext.allocator = vma::createAllocator(allocatorCreateInfo);

    CreateSwapChain();
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

    for (auto& textureResource : GTextures)
    {
        VulkanTexture& texture = textureResource.payload;

        vkContext.device.destroyImageView(texture.imageView);
        vkContext.allocator.destroyImage(texture.image, texture.memory);
    }

    GTextures.Clear();

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

void Zn::RHIDevice::CreateSwapChain()
{
    GSwapChain.Create();

    TTextureResource depthTexture = CreateRHITexture(RHITextureDescriptor {
        .width       = GSwapChain.extent.width,
        .height      = GSwapChain.extent.height,
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

    depthTexture.payload.imageView = VulkanContext::Get().device.createImageView(depthTextureViewCreateInfo);

    GDepthTextureHandle = GTextures.Add(std::move(depthTexture));
}

void RHIDevice::CleanupSwapChain()
{
    VulkanContext& vkContext = VulkanContext::Get();

    if (TTextureResource* depthTexture = GTextures[GDepthTextureHandle])
    {
        vkContext.device.destroyImageView(depthTexture->payload.imageView);
        vkContext.allocator.destroyImage(depthTexture->payload.image, depthTexture->payload.memory);

        GTextures.Evict(GDepthTextureHandle);

        GDepthTextureHandle = TextureHandle();
    }

    GSwapChain.Destroy();
}

void RHIDevice::CreateFrameBuffers()
{
    vk::FramebufferCreateInfo frameBufferCreate {
        .renderPass = GVkRenderPass,
        .width      = GSwapChain.extent.width,
        .height     = GSwapChain.extent.height,
        .layers     = 1,
    };

    GVkFrameBuffers = Vector<vk::Framebuffer>(GSwapChain.imageCount);

    TTextureResource* depthTexture = GTextures[GDepthTextureHandle];

    check(depthTexture);

    for (uint32 index = 0; index < GSwapChain.imageCount; ++index)
    {
        vk::ImageView attachments[2] = {GSwapChain.imageViews[index], depthTexture->payload.imageView};

        frameBufferCreate.setAttachments(attachments);

        GVkFrameBuffers[index] = VulkanContext::Get().device.createFramebuffer(frameBufferCreate);
    }
}

void RHIDevice::CleanupFrameBuffers()
{
    for (vk::Framebuffer& frameBuffer : GVkFrameBuffers)
    {
        VulkanContext::Get().device.destroyFramebuffer(frameBuffer);
    }
}
