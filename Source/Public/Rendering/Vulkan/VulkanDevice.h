#pragma once

#include <optional>
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

		void Deinitialize();

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

		VkInstance m_VkInstance; // Vulkan library handle
		VkDevice m_VkDevice; // Vulkan Device to issue commands
		VkPhysicalDevice m_VkGPU{ VK_NULL_HANDLE }; // Graphics Card Handle
		VkDebugUtilsMessengerEXT m_DebugMessenger; // Debug message handler

		VkSurfaceKHR m_VkSurface;
		VkQueue m_VkGraphicsQueue;
		VkQueue m_VkPresentQueue;
		VkSwapchainKHR m_VkSwapChain;

		VkSurfaceFormatKHR m_VkSwapChainFormat;
		VkExtent2D m_VkSwapChainExtent;

		Vector<VkImage> m_VkSwapChainImages;
		Vector<VkImageView> m_VkImageViews;

		VkCommandPool m_VkCommandPool;
		VkCommandBuffer m_VkCommandBuffer;

		VkRenderPass m_VkRenderPass;
		Vector<VkFramebuffer> m_VkFramebuffers;
		
		VkSemaphore m_VkPresentSemaphore, m_VkRenderSemaphore;
		VkFence m_VkRenderFence;

		VkDescriptorPool m_VkImGuiDescriptorPool;

		static const Vector<const char*> kValidationLayers;

		static const Vector<const char*> kDeviceExtensions;
	};
}
