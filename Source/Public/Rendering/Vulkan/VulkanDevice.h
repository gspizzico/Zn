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

		VkShaderModule CreateShaderModule(const Vector<uint8>& InBytes);

		void CreateDescriptors();
		void CreateSwapChain();
		void CreateImageViews();
		void CreateFramebuffers();

		void CleanupSwapChain();

		void RecreateSwapChain();

		bool m_IsInitialized{ false };

		bool m_IsMinimized{ false };

		uint32 m_WindowID = 0;

		size_t m_CurrentFrame = 0;

		size_t m_FrameNumber = 0;

		u32 m_SwapChainImageIndex = 0;

		vk::Instance instance;
		vk::SurfaceKHR surface;

		// VkDevice m_VkDevice{ VK_NULL_HANDLE }; // Vulkan Device to issue commands
		vk::Device device;
		vk::PhysicalDevice gpu{ VK_NULL_HANDLE }; // Graphics Card Handle
		VkDebugUtilsMessengerEXT m_DebugMessenger{ VK_NULL_HANDLE }; // Debug message handler

		vk::Queue graphicsQueue{ VK_NULL_HANDLE };
		vk::Queue presentQueue{ VK_NULL_HANDLE };
		vk::SwapchainKHR swapChain{ VK_NULL_HANDLE };

		vk::SurfaceFormatKHR swapChainFormat{};
		vk::Extent2D swapChainExtent{};

		Vector<vk::Image> swapChainImages;
		Vector<vk::ImageView> swapChainImageViews;

		vk::CommandPool commandPool;
		// VkCommandPool m_VkCommandPool{ VK_NULL_HANDLE };
		// VkCommandBuffer m_VkCommandBuffers[kMaxFramesInFlight]{VK_NULL_HANDLE, VK_NULL_HANDLE};
		Vector<vk::CommandBuffer> commandBuffers;

		VkRenderPass m_VkRenderPass{ VK_NULL_HANDLE };
		Vector<vk::Framebuffer> frameBuffers{};
		
		VkSemaphore m_VkPresentSemaphores[kMaxFramesInFlight], m_VkRenderSemaphores[kMaxFramesInFlight];
		VkFence m_VkRenderFences[kMaxFramesInFlight];

		vk::DescriptorPool descriptorPool{};
		vk::DescriptorSetLayout globalDescriptorSetLayout{};
		Vector<vk::DescriptorSet> globalDescriptorSets;

		VkDescriptorPool m_VkImGuiDescriptorPool{VK_NULL_HANDLE};

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

		DestroyQueue m_DestroyQueue{};

		vma::Allocator allocator{VK_NULL_HANDLE};

		Vk::AllocatedBuffer CreateBuffer(size_t size, vk::BufferUsageFlags usage, vma::MemoryUsage memoryUsage);
		void DestroyBuffer(Vk::AllocatedBuffer buffer);

		// TODO: Naming might be incorrect
		void CopyToGPU(vma::Allocation allocation, void* src, size_t size);

		// == Depth Buffer ==

		vk::ImageView depthImageView;

		Vk::AllocatedImage m_DepthImage;

		// Format of the depth image.
		vk::Format depthImageFormat;

		// ==================

		// == Scene Management ==

		Vector<Vk::RenderObject> m_Renderables;		
		UnorderedMap<String, Vk::Mesh> m_Meshes;
		
		Vk::Mesh* GetMesh(const String& InName);

		void DrawObjects(VkCommandBuffer InCommandBuffer, Vk::RenderObject* InFirst, int32 InCount);

		void CreateScene();

		// ==================

		// == Camera ==
		glm::vec3 camera_position{ 0.f, 0.f, 0.f };
		glm::vec3 camera_direction { 0.0f, 0.0f, 1.f };
		glm::vec3 up_vector{ 0.0f, 1.f, 0.f };

		Vk::AllocatedBuffer m_CameraBuffer[kMaxFramesInFlight];
		Vk::AllocatedBuffer lighting_buffer[kMaxFramesInFlight];

		// ==================

		void LoadMeshes();

		void UploadMesh(Vk::Mesh& OutMesh);

		void CreateMeshPipeline();

		// ===================

		// == Texture ==

		Vk::AllocatedImage CreateTexture(const String& texture);
		Vk::AllocatedBuffer create_staging_texture(const Vk::RawTexture& inRawTexture);
		Vk::AllocatedImage CreateTextureImage(u32 width, u32 height, const Vk::AllocatedBuffer& inStagingTexture);
		void TransitionImageLayout(VkCommandBuffer cmd, VkImage img, VkFormat fmt, VkImageLayout prevLayout, VkImageLayout newLayout);

		UnorderedMap<String, Vk::AllocatedImage> textures;

		vk::DescriptorSetLayout singleTextureSetLayout;

		// ===================

		// == Command Buffer ==

		struct UploadContext
		{
			vk::CommandPool commandPool{ VK_NULL_HANDLE };
			VkCommandBuffer cmdBuffer{ VK_NULL_HANDLE };
			VkFence fence{ VK_NULL_HANDLE };
		};
		
		UploadContext uploadContext{};

		VkCommandBufferBeginInfo CreateCmdBufferBeginInfo(VkCommandBufferUsageFlags flags = 0);
		VkSubmitInfo CreateCmdBufferSubmitInfo(VkCommandBuffer* cmdBuffer);
		
		void ImmediateSubmit(std::function<void(VkCommandBuffer)>&& function);

		void CopyBuffer(VkCommandBuffer cmd, VkBuffer src, VkBuffer dst, VkDeviceSize size);
		void CopyBufferToImage(VkCommandBuffer cmd, VkBuffer buffer, VkImage img, u32 width, u32 height);

		// ===================

		template<typename TypePtr, typename OwnerType, typename CreateInfoType, typename VkCreateFunction, typename VkDestroyFunction>
		void CreateVkObject(OwnerType Owner, TypePtr& OutObject, const CreateInfoType& CreateInfo, VkCreateFunction&& Create, VkDestroyFunction&& Destroy);
	};	
}
