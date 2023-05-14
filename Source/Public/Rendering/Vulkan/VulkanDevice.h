#pragma once

#include <Core/Containers/Map.h>
#include <Rendering/RHI/RHITypes.h>
#include <Rendering/RHI/Vulkan/Vulkan.h>
#include <Rendering/Vulkan/VulkanTypes.h>
#include <deque>
#include <functional>
#include <optional>

struct SDL_Window;

namespace Zn
{
struct RHITexture;
struct RHIMesh;
class Texture;
struct ResourceHandle;
struct TextureSampler;
struct RHIPrimitiveGPU;

class VulkanDevice
{
  public:
    VulkanDevice();

    ~VulkanDevice();

    void Initialize(SDL_Window* InWindowHandle);

    void Cleanup();

    void BeginFrame();

    void Draw();

    void EndFrame();

    void ResizeWindow();

    void OnWindowMinimized();

    void OnWindowRestored();

  private:
    friend class VulkanRenderer;

    static constexpr u64 kWaitTimeOneSecond = 1000000000;

    static constexpr size_t kMaxFramesInFlight = 2;

    bool HasRequiredDeviceExtensions(vk::PhysicalDevice inDevice) const;

    vk::PhysicalDevice SelectPhysicalDevice(const Vector<vk::PhysicalDevice>& inDevices) const;

    QueueFamilyIndices GetQueueFamilyIndices(vk::PhysicalDevice inDevice) const;

    SwapChainDetails GetSwapChainDetails(vk::PhysicalDevice inGPU) const;

    Vector<vk::DeviceQueueCreateInfo> BuildQueueCreateInfo(const QueueFamilyIndices& InIndices) const;

    vk::ShaderModule CreateShaderModule(const Vector<uint8>& bytes);

    void CreateDescriptors();
    void CreateSwapChain();
    void CreateImageViews();
    void CreateFramebuffers();

    void CleanupSwapChain();

    void RecreateSwapChain();

    void SetViewport(vk::CommandBuffer cmd);

    bool isMinimized {false};

    uint32 windowID = 0;

    size_t currentFrame = 0;

    size_t frameNumber = 0;

    u32 swapChainImageIndex = 0;

    vk::Instance   instance;
    vk::SurfaceKHR surface;

    vk::Device                 device;
    vk::PhysicalDevice         gpu;
    vk::PhysicalDeviceFeatures gpuFeatures;

    vk::Queue        graphicsQueue;
    vk::Queue        presentQueue;
    vk::SwapchainKHR swapChain;

    vk::SurfaceFormatKHR swapChainFormat {};
    vk::Extent2D         swapChainExtent {};

    Vector<vk::Image>     swapChainImages;
    Vector<vk::ImageView> swapChainImageViews;

    vk::CommandPool           commandPool;
    Vector<vk::CommandBuffer> commandBuffers;

    vk::RenderPass          renderPass;
    Vector<vk::Framebuffer> frameBuffers {};

    vk::Semaphore presentSemaphores[kMaxFramesInFlight], renderSemaphores[kMaxFramesInFlight];
    vk::Fence     renderFences[kMaxFramesInFlight];

    vk::DescriptorPool        descriptorPool {};
    vk::DescriptorSetLayout   globalDescriptorSetLayout {};
    Vector<vk::DescriptorSet> globalDescriptorSets;

    RHIBuffer instanceData[kMaxFramesInFlight];
    RHIBuffer drawCommands[kMaxFramesInFlight];

    vk::DescriptorPool imguiDescriptorPool;

    static const Vector<const char*> kDeviceExtensions;

    class DestroyQueue
    {
      public:
        ~DestroyQueue();

        void Enqueue(std::function<void()>&& InDestructor);

        void Flush();

      private:
        std::deque<std::function<void()>> queue {};
    };

    DestroyQueue destroyQueue {};

    vma::Allocator allocator;

    RHIBuffer CreateBuffer(size_t                     size,
                           vk::BufferUsageFlags       usage,
                           vma::MemoryUsage           memoryUsage,
                           vma::AllocationCreateFlags allocationFlags = vma::AllocationCreateFlags(0)) const;

    void DestroyBuffer(RHIBuffer buffer) const;

    // TODO: Naming might be incorrect
    void CopyToGPU(vma::Allocation allocation, const void* src, size_t size) const;

    // == Scene Management ==

    Vector<RenderObject>                   renderables;
    UnorderedMap<ResourceHandle, RHIMesh*> meshes;
    Vector<RHIPrimitiveGPU*>               gpuPrimitives;

    // TODO: JUST A TEST
    UnorderedMap<RHIPrimitiveGPU*, vk::DescriptorSet> gpuPrimitivesDescriptorSets;

    RHIMesh* GetMesh(const String& InName);

    void DrawObjects(vk::CommandBuffer commandBuffer, RenderObject* first, u64 count);

    void CreateScene();

    // ==================

    // == Camera ==
    glm::vec3 cameraPosition {0.f, 0.f, 0.f};
    glm::vec3 cameraDirection {0.0f, 0.0f, 1.f};
    glm::vec3 upVector {0.0f, 1.f, 0.f};

    RHIBuffer cameraBuffer[kMaxFramesInFlight];
    RHIBuffer lightingBuffer[kMaxFramesInFlight];

    // ==================

    void CreateDefaultTexture(const String& name, const u8 (&color)[4]);

    void CreateDefaultResources();
    void LoadMeshes();

    RHIBuffer CreateRHIBuffer(const void* data, sizet size, vk::BufferUsageFlags bufferUsage, vma::MemoryUsage memoryUsage) const;

    void CreateMeshPipeline();

    // ===================

    // == Texture ==

    RHITexture* CreateTexture(const String& texture);
    RHITexture* CreateTexture(const String& name, SharedPtr<struct TextureSource> texture);

    RHITexture* CreateRHITexture(i32 width, i32 height, vk::Format format) const;
    vk::Sampler CreateSampler(const TextureSampler& sampler);

    void TransitionImageLayout(
        vk::CommandBuffer cmd, vk::Image img, vk::Format fmt, vk::ImageLayout prevLayout, vk::ImageLayout newLayout) const;

    // UnorderedMap<String, AllocatedImage> textures;
    UnorderedMap<ResourceHandle, RHITexture*> textures;

    vk::DescriptorSetLayout singleTextureSetLayout;

    // ===================

    // == Command Buffer ==

    struct UploadContext
    {
        vk::CommandPool   commandPool {VK_NULL_HANDLE};
        vk::CommandBuffer cmdBuffer {VK_NULL_HANDLE};
        vk::Fence         fence {VK_NULL_HANDLE};
    };

    UploadContext uploadContext {};

    void ImmediateSubmit(std::function<void(vk::CommandBuffer)>&& function) const;

    void CopyBufferToImage(vk::CommandBuffer cmd, vk::Buffer buffer, vk::Image img, u32 width, u32 height) const;

    vk::ImageCreateInfo     MakeImageCreateInfo(vk::Format format, vk::ImageUsageFlags usageFlags, vk::Extent3D extent) const;
    vk::ImageViewCreateInfo MakeImageViewCreateInfo(vk::Format format, vk::Image image, vk::ImageAspectFlagBits aspectFlags) const;
};
} // namespace Zn
