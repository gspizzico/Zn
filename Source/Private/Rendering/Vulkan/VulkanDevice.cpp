#include <Znpch.h>
#include <Rendering/Vulkan/VulkanDevice.h>
#include <SDL.h>
#include <SDL_vulkan.h>

#include <Core/Containers/Set.h>

#include <algorithm>

#define ZN_VK_VALIDATION_LAYERS (ZN_DEBUG)

#define ZN_VK_VALIDATION_VERBOSE (0)

// Verbosity set to ::Verbose, since messages are filtered through flags using ZN_VK_VALIDATION_VERBOSE
DEFINE_STATIC_LOG_CATEGORY(LogVulkanValidation, ELogVerbosity::Verbose);

using namespace Zn;

namespace
{
	ELogVerbosity VkMessageSeverityToZnVerbosity(VkDebugUtilsMessageSeverityFlagBitsEXT InSeverity)
	{
		switch (InSeverity)
		{
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
			return ELogVerbosity::Verbose;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
			return ELogVerbosity::Log;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
			return ELogVerbosity::Warning;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
			default:
			return ELogVerbosity::Error;
		}
	}

	const String& VkMessageTypeToString(VkDebugUtilsMessageTypeFlagBitsEXT InType)
	{
		static const String kGeneral = "VkGeneral";
		static const String kValidation = "VkValidation";
		static const String kPerformance = "VkPerf";

		switch (InType)
		{
			case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
			return kGeneral;
			case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
			return kValidation;
			case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
			return kPerformance;
			default:
			return kGeneral;
		}
	}

	template<typename OutputType, typename VkFunction, typename ...Args>
	Vector<OutputType> VkEnumerate(VkFunction&& InFunction, Args&&... InArgs)
	{
		uint32 Count = 0;
		InFunction(std::forward<Args>(InArgs)..., &Count, nullptr);

		Vector<OutputType> Output(Count);
		InFunction(std::forward<Args>(InArgs)..., &Count, Output.data());

		return Output;
	}
}

const Vector<const char*> VulkanDevice::kValidationLayers = { "VK_LAYER_KHRONOS_validation" };
const Vector<const char*> VulkanDevice::kDeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

VulkanDevice::VulkanDevice()
{

}

VulkanDevice::~VulkanDevice()
{
	Deinitialize();
}

void VulkanDevice::Initialize(SDL_Window* InWindowHandle)
{
	/////// Create VkIntance

	VkApplicationInfo ApplicationInfo{};
	ApplicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	ApplicationInfo.pApplicationName = "Zn";
	ApplicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	ApplicationInfo.pEngineName = "Zn";
	ApplicationInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	ApplicationInfo.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo CreateInfo{};
	CreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	CreateInfo.pApplicationInfo = &ApplicationInfo;

	const Vector<const char*> RequiredExtensions = GetRequiredVkExtensions(InWindowHandle);

	CreateInfo.enabledExtensionCount = static_cast<uint32>(RequiredExtensions.size());
	CreateInfo.ppEnabledExtensionNames = RequiredExtensions.data();

#if ZN_VK_VALIDATION_LAYERS
	// Enable Validation Layers
	_ASSERT(SupportsValidationLayers());

	CreateInfo.enabledLayerCount = static_cast<uint32>(kValidationLayers.size());
	CreateInfo.ppEnabledLayerNames = kValidationLayers.data();

	VkDebugUtilsMessengerCreateInfoEXT DebugCreateInfo = GetDebugMessengerCreateInfo();

	CreateInfo.pNext = &DebugCreateInfo;
#else
	CreateInfo.enabledLayerCount = 0;
#endif


	if (vkCreateInstance(&CreateInfo, nullptr, &m_VkInstance) != VK_SUCCESS)
	{
		_ASSERT(false);
		return;
	}

#if ZN_VK_VALIDATION_LAYERS
	InitializeDebugMessenger();
#endif

	/////// Create Surface
	if (SDL_Vulkan_CreateSurface(InWindowHandle, m_VkInstance, &m_VkSurface) != SDL_TRUE)
	{
		_ASSERT(false);
		return;
	}

	/////// Initialize GPU

	Vector<VkPhysicalDevice> Devices = VkEnumerate<VkPhysicalDevice>(vkEnumeratePhysicalDevices, m_VkInstance);

	m_VkGPU = SelectPhysicalDevice(Devices);

	_ASSERT(m_VkGPU != VK_NULL_HANDLE);

	/////// Initialize Logical Device

	Vk::QueueFamilyIndices Indices = GetQueueFamilyIndices(m_VkGPU);

	Vk::SwapChainDetails SwapChainDetails = GetSwapChainDetails(m_VkGPU);

	const bool IsSupported = SwapChainDetails.Formats.size() > 0 && SwapChainDetails.PresentModes.size() > 0;

	if (!IsSupported)
	{
		_ASSERT(false);
		return;
	}

	Vector<VkDeviceQueueCreateInfo> QueueFamilies = BuildQueueCreateInfo(Indices);

	VkPhysicalDeviceFeatures DeviceFeatures{}; // TEMP

	VkDeviceCreateInfo LogicalDeviceCreateInfo{};
	LogicalDeviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	LogicalDeviceCreateInfo.pQueueCreateInfos = QueueFamilies.data();
	LogicalDeviceCreateInfo.queueCreateInfoCount = static_cast<uint32>(QueueFamilies.size());
	LogicalDeviceCreateInfo.pEnabledFeatures = &DeviceFeatures;

	LogicalDeviceCreateInfo.ppEnabledExtensionNames = kDeviceExtensions.data();
	LogicalDeviceCreateInfo.enabledExtensionCount = static_cast<uint32>(kDeviceExtensions.size());

	if (vkCreateDevice(m_VkGPU, &LogicalDeviceCreateInfo, nullptr, &m_VkDevice) != VK_SUCCESS)
	{
		_ASSERT(false);
		return;
	}

	vkGetDeviceQueue(m_VkDevice, Indices.Graphics.value(), 0, &m_VkGraphicsQueue);
	vkGetDeviceQueue(m_VkDevice, Indices.Present.value(), 0, &m_VkPresentQueue);

	/////// Create Swap Chain

	VkSurfaceFormatKHR Format{ VK_FORMAT_UNDEFINED, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };

	for (const VkSurfaceFormatKHR& AvailableFormat : SwapChainDetails.Formats)
	{
		if (AvailableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && AvailableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			Format = AvailableFormat;
			break;
		}
	}

	if (Format.format == VK_FORMAT_UNDEFINED)
	{
		Format = SwapChainDetails.Formats[0];
	}

	// Always guaranteed to be available.
	VkPresentModeKHR PresentMode = VK_PRESENT_MODE_FIFO_KHR;

	const bool UseMailboxPresentMode = std::any_of(SwapChainDetails.PresentModes.begin(), SwapChainDetails.PresentModes.end(),
												   [](VkPresentModeKHR InPresentMode)
												   {
													   // 'Triple Buffering'
													   return InPresentMode == VK_PRESENT_MODE_MAILBOX_KHR;
												   });

	if (UseMailboxPresentMode)
	{
		PresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
	}

	int32 Width, Height = 0;
	SDL_Vulkan_GetDrawableSize(InWindowHandle, &Width, &Height);

	Width = std::clamp(Width, static_cast<int32>(SwapChainDetails.Capabilities.minImageExtent.width), static_cast<int32>(SwapChainDetails.Capabilities.maxImageExtent.width));
	Height = std::clamp(Height, static_cast<int32>(SwapChainDetails.Capabilities.minImageExtent.height), static_cast<int32>(SwapChainDetails.Capabilities.maxImageExtent.height));

	uint32 ImageCount = SwapChainDetails.Capabilities.minImageCount + 1;
	if (SwapChainDetails.Capabilities.maxImageCount > 0)
	{
		ImageCount = std::min(ImageCount, SwapChainDetails.Capabilities.maxImageCount);
	}

	m_VkSwapChainFormat = Format;

	m_VkSwapChainExtent = VkExtent2D(Width, Height);

	VkSwapchainCreateInfoKHR SwapChainCreateInfo{};
	SwapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	SwapChainCreateInfo.surface = m_VkSurface;
	SwapChainCreateInfo.minImageCount = ImageCount;
	SwapChainCreateInfo.imageFormat = m_VkSwapChainFormat.format;
	SwapChainCreateInfo.imageColorSpace = m_VkSwapChainFormat.colorSpace;
	SwapChainCreateInfo.imageExtent = m_VkSwapChainExtent;
	SwapChainCreateInfo.imageArrayLayers = 1;
	SwapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	uint32 QueueFamilyIndicesArray[] = { Indices.Graphics.value(), Indices.Present.value() };

	if (Indices.Graphics.value() != Indices.Present.value())
	{
		SwapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		SwapChainCreateInfo.queueFamilyIndexCount = 2;
		SwapChainCreateInfo.pQueueFamilyIndices = QueueFamilyIndicesArray;
	}
	else
	{
		SwapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		SwapChainCreateInfo.queueFamilyIndexCount = 0;
		SwapChainCreateInfo.pQueueFamilyIndices = nullptr;
	}

	SwapChainCreateInfo.preTransform = SwapChainDetails.Capabilities.currentTransform;
	SwapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

	SwapChainCreateInfo.presentMode = PresentMode;
	SwapChainCreateInfo.clipped = true;

	if (vkCreateSwapchainKHR(m_VkDevice, &SwapChainCreateInfo, nullptr/*allocator*/, &m_VkSwapChain) != VK_SUCCESS)
	{
		_ASSERT(false);
		return;
	}

	m_VkSwapChainImages = VkEnumerate<VkImage>(vkGetSwapchainImagesKHR, m_VkSwapChain);

	m_VkImageViews.resize(m_VkSwapChainImages.size());

	for (size_t Index = 0; Index < m_VkSwapChainImages.size(); ++Index)
	{
		VkImageViewCreateInfo ImageViewCreateInfo{};
		ImageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		ImageViewCreateInfo.image = m_VkSwapChainImages[Index];
		ImageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		ImageViewCreateInfo.format = m_VkSwapChainFormat.format;

		// RGBA
		ImageViewCreateInfo.components = { 
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY 
		};

		ImageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		ImageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		ImageViewCreateInfo.subresourceRange.levelCount = 1;
		ImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		ImageViewCreateInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(m_VkDevice, &ImageViewCreateInfo, nullptr/*allocator*/, &m_VkImageViews[Index]) != VK_SUCCESS)
		{
			_ASSERT(false);
			return;
		}
	}
}

void VulkanDevice::Deinitialize()
{
#if ZN_VK_VALIDATION_LAYERS
	DeinitializeDebugMessenger();
#endif

	for (VkImageView& ImageView : m_VkImageViews)
	{
		vkDestroyImageView(m_VkDevice, ImageView, nullptr/*allocator*/);
	}

	m_VkImageViews.empty();

	if (m_VkSwapChain != NULL)
	{
		vkDestroySwapchainKHR(m_VkDevice, m_VkSwapChain, nullptr/*allocator*/);
		m_VkSwapChain = NULL;
	}

	if (m_VkSurface != NULL)
	{
		vkDestroySurfaceKHR(m_VkInstance, m_VkSurface, nullptr/*allocator*/);
		m_VkSurface = NULL;
	}

	if (m_VkDevice != NULL)
	{
		vkDestroyDevice(m_VkDevice, nullptr/*allocator*/);
		m_VkDevice = NULL;
	}

	if (m_VkInstance != NULL)
	{
		vkDestroyInstance(m_VkInstance, nullptr/*allocator*/);
		m_VkInstance = NULL;
	}
}

bool VulkanDevice::SupportsValidationLayers() const
{
	//Vector<VkLayerProperties> AvailableLayers = VkEnumerate<VkLayerProperties, VkResult>(vkEnumerateInstanceLayerProperties);
	Vector<VkLayerProperties> AvailableLayers = VkEnumerate<VkLayerProperties>(vkEnumerateInstanceLayerProperties);

	return std::any_of(AvailableLayers.begin(), AvailableLayers.end(), [](const VkLayerProperties& It)
	{
		for (const auto& LayerName : kValidationLayers)
		{
			if (strcmp(It.layerName, LayerName) == 0)
			{
				return true;
			}
		}

		return false;
	});
}

Vector<const char*> VulkanDevice::GetRequiredVkExtensions(SDL_Window* InWindowHandle) const
{
	uint32 ExtensionsCount = 0;
	SDL_Vulkan_GetInstanceExtensions(InWindowHandle, &ExtensionsCount, nullptr);
	Vector<const char*> ExtensionsNames(ExtensionsCount);
	SDL_Vulkan_GetInstanceExtensions(InWindowHandle, &ExtensionsCount, ExtensionsNames.data());

#if ZN_VK_VALIDATION_LAYERS
	static const Vector<const char*> kRequiredExtensions = { VK_EXT_DEBUG_UTILS_EXTENSION_NAME };

	ExtensionsCount += static_cast<uint32>(kRequiredExtensions.size());
	ExtensionsNames.insert(ExtensionsNames.end(), kRequiredExtensions.begin(), kRequiredExtensions.end());
#endif

	return ExtensionsNames;

}

bool VulkanDevice::HasRequiredDeviceExtensions(VkPhysicalDevice InDevice) const
{
	Vector<VkExtensionProperties> AvailableExtensions = VkEnumerate<VkExtensionProperties>(vkEnumerateDeviceExtensionProperties, InDevice, nullptr);

	static const Set<String> kRequiredDeviceExtensions(kDeviceExtensions.begin(), kDeviceExtensions.end());

	size_t NumFoundExtensions =
		std::count_if(AvailableExtensions.begin(), AvailableExtensions.end(), [](const VkExtensionProperties& InExtension)
		{
			return kRequiredDeviceExtensions.contains(InExtension.extensionName);
		});

	return kRequiredDeviceExtensions.size() == NumFoundExtensions;
}

VkDebugUtilsMessengerCreateInfoEXT VulkanDevice::GetDebugMessengerCreateInfo() const
{
	VkDebugUtilsMessengerCreateInfoEXT DebugCreateInfo{};

	DebugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
#if ZN_VK_VALIDATION_VERBOSE
	DebugCreateInfo.messageSeverity = (VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT);
#else
	DebugCreateInfo.messageSeverity = (VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT);
#endif
	DebugCreateInfo.messageType = (VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT);
	DebugCreateInfo.pfnUserCallback = OnDebugMessage;
	DebugCreateInfo.pUserData = nullptr;

	return DebugCreateInfo;
}

void VulkanDevice::InitializeDebugMessenger()
{
	VkDebugUtilsMessengerCreateInfoEXT DebugUtilsInfo = GetDebugMessengerCreateInfo();

	// Load function
	auto vkCreateDebugUtilsMessengerEXTPtr = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(m_VkInstance, "vkCreateDebugUtilsMessengerEXT");
	if (vkCreateDebugUtilsMessengerEXTPtr != nullptr)
	{
		vkCreateDebugUtilsMessengerEXTPtr(m_VkInstance, &DebugUtilsInfo, nullptr/*Allocator*/, &m_DebugMessenger);
	}
	else
	{
		ZN_LOG(LogVulkanValidation, ELogVerbosity::Error, "vkCreateDebugUtilsMessengerEXT not available.");
	}
}

void VulkanDevice::DeinitializeDebugMessenger()
{
	if (m_DebugMessenger != NULL)
	{
		// Load function
		auto vkDestroyDebugUtilsMessengerEXTPtr = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(m_VkInstance, "vkDestroyDebugUtilsMessengerEXT");
		if (vkDestroyDebugUtilsMessengerEXTPtr != nullptr)
		{
			vkDestroyDebugUtilsMessengerEXTPtr(m_VkInstance, m_DebugMessenger, nullptr/*Allocator*/);

			m_DebugMessenger = NULL;
		}
	}
}

VkBool32 VulkanDevice::OnDebugMessage(VkDebugUtilsMessageSeverityFlagBitsEXT Severity,
									  VkDebugUtilsMessageTypeFlagsEXT Type,
									  const VkDebugUtilsMessengerCallbackDataEXT* Data,
									  void* UserData)
{
	ELogVerbosity Verbosity = VkMessageSeverityToZnVerbosity(Severity);

	const String& MessageType = VkMessageTypeToString(static_cast<VkDebugUtilsMessageTypeFlagBitsEXT>(Type));

	ZN_LOG(LogVulkanValidation, Verbosity, "[%s] %s", MessageType.c_str(), Data->pMessage);

	return VK_FALSE;
}

VkPhysicalDevice VulkanDevice::SelectPhysicalDevice(const Vector<VkPhysicalDevice>& InDevices) const
{
	size_t NumDevices = InDevices.size();

	size_t BestIndex = std::numeric_limits<size_t>::max();
	uint32 MaxScore = 0;

	for (size_t Index = 0; Index < NumDevices; ++Index)
	{
		VkPhysicalDevice Device = InDevices[Index];

		uint32 Score = 0;

		const bool HasGraphicsQueue = GetQueueFamilyIndices(Device).Graphics.has_value();

		const bool HasRequiredExtensions = HasRequiredDeviceExtensions(Device);

		if (HasGraphicsQueue && HasRequiredExtensions)
		{
			VkPhysicalDeviceProperties Properties;
			VkPhysicalDeviceFeatures Features;
			vkGetPhysicalDeviceProperties(Device, &Properties);
			vkGetPhysicalDeviceFeatures(Device, &Features);

			if (Properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			{
				Score += 1000;
			}

			// Texture size influences quality
			Score += Properties.limits.maxImageDimension2D;

			// TODO: Add more criteria to choose GPU.
		}

		if (Score > MaxScore && Score != 0)
		{
			MaxScore = Score;
			BestIndex = Index;
		}
	}

	if (BestIndex != std::numeric_limits<size_t>::max())
	{
		return InDevices[BestIndex];
	}
	else
	{
		return VK_NULL_HANDLE;
	}
}

Vk::QueueFamilyIndices VulkanDevice::GetQueueFamilyIndices(VkPhysicalDevice InDevice) const
{
	Vk::QueueFamilyIndices Indices;

	//Vector<VkQueueFamilyProperties> QueueFamilies = VkEnumerate<VkQueueFamilyProperties>(InDevice, vkGetPhysicalDeviceQueueFamilyProperties);
	Vector<VkQueueFamilyProperties> QueueFamilies = VkEnumerate<VkQueueFamilyProperties>(vkGetPhysicalDeviceQueueFamilyProperties, InDevice);

	for (size_t Index = 0; Index < QueueFamilies.size(); ++Index)
	{
		const VkQueueFamilyProperties& QueueFamily = QueueFamilies[Index];

		if (QueueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			Indices.Graphics = static_cast<uint32>(Index);
		}

		VkBool32 PresentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(InDevice, Index, m_VkSurface, &PresentSupport);

		// tbd: We could enforce that Graphics and Present are in the same queue but is not mandatory.
		if (PresentSupport)
		{
			Indices.Present = static_cast<uint32>(Index);
		}
	}

	return Indices;
}

Vk::SwapChainDetails VulkanDevice::GetSwapChainDetails(VkPhysicalDevice InDevice) const
{
	Vk::SwapChainDetails Details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(InDevice, m_VkSurface, &Details.Capabilities);

	Details.Formats = VkEnumerate<VkSurfaceFormatKHR>(vkGetPhysicalDeviceSurfaceFormatsKHR, InDevice, m_VkSurface);
	Details.PresentModes = VkEnumerate<VkPresentModeKHR>(vkGetPhysicalDeviceSurfacePresentModesKHR, InDevice, m_VkSurface);

	return Details;
}

Vector<VkDeviceQueueCreateInfo> VulkanDevice::BuildQueueCreateInfo(const Vk::QueueFamilyIndices& InIndices) const
{
	Set<uint32> Queues = { InIndices.Graphics.value(), InIndices.Present.value() };

	Vector<VkDeviceQueueCreateInfo> OutCreateInfo{};
	OutCreateInfo.reserve(Queues.size());

	for (uint32 QueueIndex : Queues)
	{
		VkDeviceQueueCreateInfo DeviceQueueCreateInfo{};
		DeviceQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		DeviceQueueCreateInfo.queueFamilyIndex = QueueIndex;
		DeviceQueueCreateInfo.queueCount = 1;

		static const float kQueuePriority = 1.0f;

		DeviceQueueCreateInfo.pQueuePriorities = &kQueuePriority;

		OutCreateInfo.emplace_back(std::move(DeviceQueueCreateInfo));
	}

	return OutCreateInfo;
}