#pragma once

#include <optional>
#include <deque>
#include <functional>
#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>
#include <Core/Containers/Map.h>
#include <Rendering/Vulkan/VulkanTypes.h>
#include <Rendering/Vulkan/VulkanMesh.h>

#include <glm/glm.hpp>
#include <glm/ext.hpp>

struct SDL_Window;

namespace Zn
{
	class VulkanDevice
	{

	public:

		VulkanDevice();

		~VulkanDevice();

		void Initialize(SDL_Window* InWindowHandle, vk::Instance inInstance, vk::SurfaceKHR inSurface);

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

		Vk::QueueFamilyIndices GetQueueFamilyIndices(vk::PhysicalDevice inDevice) const;

		Vk::SwapChainDetails GetSwapChainDetails(vk::PhysicalDevice inGPU) const;

		Vector<vk::DeviceQueueCreateInfo> BuildQueueCreateInfo(const Vk::QueueFamilyIndices& InIndices) const;

		vk::ShaderModule CreateShaderModule(const Vector<uint8>& bytes);

		void CreateDescriptors();
		void CreateSwapChain();
		void CreateImageViews();
		void CreateFramebuffers();

		void CleanupSwapChain();

		void RecreateSwapChain();

		bool isInitialized{ false };

		bool isMinimized{ false };

		uint32 windowID = 0;

		size_t currentFrame = 0;

		size_t frameNumber = 0;

		u32 swapChainImageIndex = 0;

		vk::Instance instance;
		vk::SurfaceKHR surface;

		vk::Device device;
		vk::PhysicalDevice gpu;

		vk::Queue graphicsQueue;
		vk::Queue presentQueue;
		vk::SwapchainKHR swapChain;

		vk::SurfaceFormatKHR swapChainFormat{};
		vk::Extent2D swapChainExtent{};

		Vector<vk::Image> swapChainImages;
		Vector<vk::ImageView> swapChainImageViews;

		vk::CommandPool commandPool;
		Vector<vk::CommandBuffer> commandBuffers;

		vk::RenderPass renderPass;
		Vector<vk::Framebuffer> frameBuffers{};
		
		vk::Semaphore presentSemaphores[kMaxFramesInFlight], renderSemaphores[kMaxFramesInFlight];
		vk::Fence renderFences[kMaxFramesInFlight];

		vk::DescriptorPool descriptorPool{};
		vk::DescriptorSetLayout globalDescriptorSetLayout{};
		Vector<vk::DescriptorSet> globalDescriptorSets;

		vk::DescriptorPool imguiDescriptorPool;

		static const Vector<const char*> kDeviceExtensions;

		class DestroyQueue
		{
		public:

			~DestroyQueue();

			void Enqueue(std::function<void()>&& InDestructor);

			void Flush();

		private:

			std::deque<std::function<void()>> m_Queue{};
		};

		DestroyQueue destroyQueue{};

		vma::Allocator allocator;

		Vk::AllocatedBuffer CreateBuffer(size_t size, vk::BufferUsageFlags usage, vma::MemoryUsage memoryUsage);
		void DestroyBuffer(Vk::AllocatedBuffer buffer);

		// TODO: Naming might be incorrect
		void CopyToGPU(vma::Allocation allocation, void* src, size_t size);

		// == Depth Buffer ==

		vk::ImageView depthImageView;

		Vk::AllocatedImage depthImage;

		// Format of the depth image.
		vk::Format depthImageFormat;

		// ==================

		// == Scene Management ==

		Vector<Vk::RenderObject> renderables;		
		UnorderedMap<String, Vk::Mesh> meshes;
		
		Vk::Mesh* GetMesh(const String& InName);

		void DrawObjects(vk::CommandBuffer commandBuffer, Vk::RenderObject* first, int32 count);

		void CreateScene();

		// ==================

		// == Camera ==
		glm::vec3 cameraPosition{ 0.f, 0.f, 0.f };
		glm::vec3 cameraDirection { 0.0f, 0.0f, 1.f };
		glm::vec3 upVector{ 0.0f, 1.f, 0.f };

		Vk::AllocatedBuffer cameraBuffer[kMaxFramesInFlight];
		Vk::AllocatedBuffer lightingBuffer[kMaxFramesInFlight];

		// ==================

		void LoadMeshes();

		void UploadMesh(Vk::Mesh& OutMesh);

		void CreateMeshPipeline();

		// ===================

		// == Texture ==

		Vk::AllocatedImage CreateTexture(const String& texture);
		Vk::AllocatedImage CreateTextureImage(u32 width, u32 height, const Vk::AllocatedBuffer& inStagingTexture);
		void TransitionImageLayout(vk::CommandBuffer cmd, vk::Image img, vk::Format fmt, vk::ImageLayout prevLayout, vk::ImageLayout newLayout);

		UnorderedMap<String, Vk::AllocatedImage> textures;

		vk::DescriptorSetLayout singleTextureSetLayout;

		// ===================

		// == Command Buffer ==

		struct UploadContext
		{
			vk::CommandPool commandPool{ VK_NULL_HANDLE };
			vk::CommandBuffer cmdBuffer{ VK_NULL_HANDLE };
			vk::Fence fence{ VK_NULL_HANDLE };
		};
		
		UploadContext uploadContext{};
				
		void ImmediateSubmit(std::function<void(vk::CommandBuffer)>&& function);

		void CopyBufferToImage(vk::CommandBuffer cmd, vk::Buffer buffer, vk::Image img, u32 width, u32 height);

		// ===================

		template<typename TypePtr, typename OwnerType, typename CreateInfoType, typename VkCreateFunction, typename VkDestroyFunction>
		void CreateVkObject(OwnerType Owner, TypePtr& OutObject, const CreateInfoType& CreateInfo, VkCreateFunction&& Create, VkDestroyFunction&& Destroy);
	};	
}
