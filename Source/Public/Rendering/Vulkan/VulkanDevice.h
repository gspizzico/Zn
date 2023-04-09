#pragma once

#include <optional>
#include <deque>
#include <functional>
#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>
#include <Rendering/Vulkan/VulkanTypes.h>
#include <Rendering/Vulkan/VulkanMesh.h>

struct SDL_Window;

namespace Zn
{

	class VulkanDevice
	{

	public:

		VulkanDevice();

		~VulkanDevice();

		void Initialize(SDL_Window* InWindowHandle);

		void Cleanup();

		void Draw();

		void ResizeWindow();

		void OnWindowMinimized();

		void OnWindowRestored();

	private:

		static constexpr size_t kMaxFramesInFlight = 2;

		bool SupportsValidationLayers() const;

		Vector<const char*> GetRequiredVkExtensions(SDL_Window* InWindowHandle) const;

		bool HasRequiredDeviceExtensions(VkPhysicalDevice InDevice) const;

		VkDebugUtilsMessengerCreateInfoEXT GetDebugMessengerCreateInfo() const;

		void InitializeDebugMessenger();

		void DeinitializeDebugMessenger();

		static VKAPI_ATTR VkBool32 VKAPI_CALL OnDebugMessage(VkDebugUtilsMessageSeverityFlagBitsEXT Severity, 
															 VkDebugUtilsMessageTypeFlagsEXT Type,
															 const VkDebugUtilsMessengerCallbackDataEXT* Data,
															 void* UserData);

		VkPhysicalDevice SelectPhysicalDevice(const Vector<VkPhysicalDevice>& InDevices) const;

		Vk::QueueFamilyIndices GetQueueFamilyIndices(VkPhysicalDevice InDevice) const;

		Vk::SwapChainDetails GetSwapChainDetails(VkPhysicalDevice InDevice) const;

		Vector<VkDeviceQueueCreateInfo> BuildQueueCreateInfo(const Vk::QueueFamilyIndices& InIndices) const;

		void LoadShaders();

		VkShaderModule CreateShaderModule(const Vector<uint8>& InBytes);

		void CreateSwapChain();
		void CreateImageViews();
		void CreateFramebuffers();

		void CleanupSwapChain();

		void RecreateSwapChain();

		bool m_IsInitialized{ false };

		bool m_IsMinimized{ false };

		size_t m_CurrentFrame = 0;

		size_t m_FrameNumber = 0;

		VkInstance m_VkInstance{VK_NULL_HANDLE}; // Vulkan library handle
		VkDevice m_VkDevice{ VK_NULL_HANDLE }; // Vulkan Device to issue commands
		VkPhysicalDevice m_VkGPU{ VK_NULL_HANDLE }; // Graphics Card Handle
		VkDebugUtilsMessengerEXT m_DebugMessenger{ VK_NULL_HANDLE }; // Debug message handler

		VkSurfaceKHR m_VkSurface{ VK_NULL_HANDLE };
		VkQueue m_VkGraphicsQueue{ VK_NULL_HANDLE };
		VkQueue m_VkPresentQueue{ VK_NULL_HANDLE };
		VkSwapchainKHR m_VkSwapChain{ VK_NULL_HANDLE };

		VkSurfaceFormatKHR m_VkSwapChainFormat{};
		VkExtent2D m_VkSwapChainExtent{};

		Vector<VkImage> m_VkSwapChainImages;
		Vector<VkImageView> m_VkImageViews;

		VkCommandPool m_VkCommandPool{ VK_NULL_HANDLE };
		VkCommandBuffer m_VkCommandBuffers[kMaxFramesInFlight]{VK_NULL_HANDLE, VK_NULL_HANDLE};

		VkRenderPass m_VkRenderPass{ VK_NULL_HANDLE };
		Vector<VkFramebuffer> m_VkFramebuffers{};
		
		VkSemaphore m_VkPresentSemaphores[kMaxFramesInFlight], m_VkRenderSemaphores[kMaxFramesInFlight];
		VkFence m_VkRenderFences[kMaxFramesInFlight];

		VkDescriptorPool m_VkImGuiDescriptorPool{VK_NULL_HANDLE};

		VkShaderModule m_VkVert{VK_NULL_HANDLE};
		VkShaderModule m_VkFrag{VK_NULL_HANDLE};
		VkShaderModule m_VkMeshVert{VK_NULL_HANDLE};

		VkPipelineLayout m_VkPipelineLayout{VK_NULL_HANDLE};
		VkPipelineLayout m_VkMeshPipelineLayout{VK_NULL_HANDLE};

		VkPipeline m_VkPipeline{ VK_NULL_HANDLE };
		VkPipeline m_VkMeshPipeline{ VK_NULL_HANDLE };

		static const Vector<const char*> kValidationLayers;

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

		VmaAllocator m_VkAllocator{VK_NULL_HANDLE};

		Vk::Mesh m_Mesh;
		Vk::Mesh m_Monkey;

		// == Depth Buffer ==

		VkImageView m_DepthImageView;
		Vk::AllocatedImage m_DepthImage;

		// Format of the depth image.
		VkFormat m_DepthFormat;

		// ==================

		void LoadMeshes();

		void UploadMesh(Vk::Mesh& OutMesh);

		void CreateMeshPipeline();

		template<typename TypePtr, typename OwnerType, typename CreateInfoType, typename VkCreateFunction, typename VkDestroyFunction>
		void CreateVkObject(OwnerType Owner, TypePtr& OutObject, const CreateInfoType& CreateInfo, VkCreateFunction&& Create, VkDestroyFunction&& Destroy);
	};	
}
