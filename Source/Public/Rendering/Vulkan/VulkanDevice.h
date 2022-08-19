#pragma once

#include <optional>
#include <deque>
#include <functional>
#include <vulkan/vulkan.h>

struct SDL_Window;

namespace Zn
{
	namespace Vk
	{
		struct QueueFamilyIndices
		{
			std::optional<uint32> Graphics;
			std::optional<uint32> Present;
		};

		struct SwapChainDetails
		{
			VkSurfaceCapabilitiesKHR Capabilities;
			Vector<VkSurfaceFormatKHR> Formats;
			Vector<VkPresentModeKHR> PresentModes;
		};
	}

	class VulkanDevice
	{

	public:

		VulkanDevice();

		~VulkanDevice();

		void Initialize(SDL_Window* InWindowHandle);

		void Cleanup();

		void Draw();

	private:

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

		bool IsInitialized{ false };

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
		VkCommandBuffer m_VkCommandBuffer{ VK_NULL_HANDLE };

		VkRenderPass m_VkRenderPass{ VK_NULL_HANDLE };
		Vector<VkFramebuffer> m_VkFramebuffers{};
		
		VkSemaphore m_VkPresentSemaphore, m_VkRenderSemaphore;
		VkFence m_VkRenderFence;

		VkDescriptorPool m_VkImGuiDescriptorPool{VK_NULL_HANDLE};

		VkShaderModule m_VkVert{VK_NULL_HANDLE};
		VkShaderModule m_VkFrag{VK_NULL_HANDLE};

		VkPipelineLayout m_VkPipelineLayout{VK_NULL_HANDLE};
		VkPipeline m_VkPipeline{ VK_NULL_HANDLE };

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

		template<typename TypePtr, typename OwnerType, typename CreateInfoType, typename VkCreateFunction, typename VkDestroyFunction>
		void CreateVkObject(OwnerType Owner, TypePtr& OutObject, const CreateInfoType& CreateInfo, VkCreateFunction&& Create, VkDestroyFunction&& Destroy);
	};	
}
