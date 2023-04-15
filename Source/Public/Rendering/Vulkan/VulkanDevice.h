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

		void Initialize(SDL_Window* InWindowHandle, VkInstance InVkInstanceHandle, VkSurfaceKHR InVkSurface);

		void Cleanup();

		void BeginFrame();

		void Draw();

		void EndFrame();

		void ResizeWindow();

		void OnWindowMinimized();

		void OnWindowRestored();

	private:

		friend class VulkanBackend;

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

		VkDescriptorPool m_VkDescriptorPool{ VK_NULL_HANDLE };
		VkDescriptorSetLayout m_VkGlobalSetLayout{ VK_NULL_HANDLE };
		VkDescriptorSet m_VkGlobalDescriptorSet[kMaxFramesInFlight];

		VkDescriptorPool m_VkImGuiDescriptorPool{VK_NULL_HANDLE};

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

		Vk::AllocatedBuffer CreateBuffer(size_t size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);		
		void DestroyBuffer(Vk::AllocatedBuffer buffer);

		// TODO: Naming might be incorrect
		void CopyToGPU(VmaAllocation allocation, void* src, size_t size);

		// == Depth Buffer ==

		VkImageView m_DepthImageView;
		Vk::AllocatedImage m_DepthImage;

		// Format of the depth image.
		VkFormat m_DepthFormat;

		// ==================

		// == Scene Management ==

		Vector<Vk::RenderObject> m_Renderables;
		UnorderedMap<String, Vk::Material> m_Materials;
		UnorderedMap<String, Vk::Mesh> m_Meshes;

		Vk::Material* CreateMaterial(VkPipeline InPipeline, VkPipelineLayout InLayout, const String& InName);
		Vk::Material* GetMaterial(const String& InName);
		Vk::Mesh* GetMesh(const String& InName);

		void DrawObjects(VkCommandBuffer InCommandBuffer, Vk::RenderObject* InFirst, int32 InCount);

		void CreateScene();

		// ==================

		// == Camera ==
		glm::vec3 camera_position{ 0.f, 0.f, -10.f };
		glm::vec3 camera_direction { 0.0f, 0.0f, -1.f };
		glm::vec3 up_vector{ 0.0f, 1.f, 0.f };

		Vk::AllocatedBuffer m_CameraBuffer[kMaxFramesInFlight];

		// ==================

		void LoadMeshes();

		void UploadMesh(Vk::Mesh& OutMesh);

		void CreateMeshPipeline();

		template<typename TypePtr, typename OwnerType, typename CreateInfoType, typename VkCreateFunction, typename VkDestroyFunction>
		void CreateVkObject(OwnerType Owner, TypePtr& OutObject, const CreateInfoType& CreateInfo, VkCreateFunction&& Create, VkDestroyFunction&& Destroy);
	};	
}
