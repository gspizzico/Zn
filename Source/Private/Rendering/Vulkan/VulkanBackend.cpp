#include "Znpch.h"
#include "Rendering/Vulkan/Vulkan.h"
#include "Rendering/Vulkan/VulkanBackend.h"
#include "Rendering/Vulkan/VulkanUtils.h"
#include "Engine/Window.h"

#include <Core/Log/LogMacros.h>

#include <algorithm>

DEFINE_STATIC_LOG_CATEGORY(LogVulkan_2, ELogVerbosity::Log);
DEFINE_STATIC_LOG_CATEGORY(LogVulkanValidation_2, ELogVerbosity::Verbose);

using namespace Zn;

static const Zn::Vector<const char*> kRequiredExtensions = { VK_EXT_DEBUG_UTILS_EXTENSION_NAME };
static const Zn::Vector<const char*> kValidationLayers = { "VK_LAYER_KHRONOS_validation" };
// static const Zn::Vector<const char*> kDeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

#if ZN_VK_VALIDATION_LAYERS
namespace VulkanValidation
{
	bool SupportsValidationLayers()
	{
		Zn::Vector<VkLayerProperties> availableLayers = VkEnumerate<VkLayerProperties>(vkEnumerateInstanceLayerProperties);

		return std::any_of(availableLayers.begin(), availableLayers.end(), [](const VkLayerProperties& it)
		{
			for (const auto& layerName : kValidationLayers)
			{
				if (strcmp(it.layerName, layerName) == 0)
				{
					return true;
				}
			}

			return false;
		});
	}

	ELogVerbosity VkMessageSeverityToZnVerbosity(VkDebugUtilsMessageSeverityFlagBitsEXT severity)
	{
		switch (severity)
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

	const String& VkMessageTypeToString(VkDebugUtilsMessageTypeFlagBitsEXT type)
	{
		static const String kGeneral = "VkGeneral";
		static const String kValidation = "VkValidation";
		static const String kPerformance = "VkPerf";

		switch (type)
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

	VKAPI_ATTR VkBool32 VKAPI_CALL OnDebugMessage(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
									  VkDebugUtilsMessageTypeFlagsEXT type,
									  const VkDebugUtilsMessengerCallbackDataEXT* data,
									  void* userData)
	{
		ELogVerbosity verbosity = VkMessageSeverityToZnVerbosity(severity);

		const String& messageType = VkMessageTypeToString(static_cast<VkDebugUtilsMessageTypeFlagBitsEXT>(type));

		ZN_LOG(LogVulkanValidation_2, verbosity, "[%s] %s", messageType.c_str(), data->pMessage);

		if (verbosity >= ELogVerbosity::Error)
		{
			__debugbreak();
		}

		return VK_FALSE;
	}

	VkDebugUtilsMessengerCreateInfoEXT GetDebugMessengerCreateInfo()
	{
		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};

		debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
#if ZN_VK_VALIDATION_VERBOSE
		debugCreateInfo.messageSeverity = (VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT);
#else
		debugCreateInfo.messageSeverity = (VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT);
#endif
		debugCreateInfo.messageType = (VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT);
		debugCreateInfo.pfnUserCallback = VulkanValidation::OnDebugMessage;
		debugCreateInfo.pUserData = nullptr;

		return debugCreateInfo;
	}

	VkDebugUtilsMessengerEXT GDebugMessenger{ VK_NULL_HANDLE };

	void InitializeDebugMessenger(VkInstance instance)
	{
		VkDebugUtilsMessengerCreateInfoEXT DebugUtilsInfo = GetDebugMessengerCreateInfo();

		// Load function
		auto vkCreateDebugUtilsMessengerEXTPtr = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
		if (vkCreateDebugUtilsMessengerEXTPtr != nullptr)
		{
			vkCreateDebugUtilsMessengerEXTPtr(instance, &DebugUtilsInfo, nullptr/*Allocator*/, &GDebugMessenger);
		}
		else
		{
			ZN_LOG(LogVulkanValidation_2, ELogVerbosity::Error, "vkCreateDebugUtilsMessengerEXT not available.");
		}
	}

	void DeinitializeDebugMessenger(VkInstance instance)
	{
		if (GDebugMessenger != VK_NULL_HANDLE)
		{
			// Load function
			auto vkDestroyDebugUtilsMessengerEXTPtr = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
			if (vkDestroyDebugUtilsMessengerEXTPtr != nullptr)
			{
				vkDestroyDebugUtilsMessengerEXTPtr(instance, GDebugMessenger, nullptr/*Allocator*/);

				GDebugMessenger = VK_NULL_HANDLE;
			}
		}
	}
}
#endif

bool Zn::VulkanBackend::initialize(RendererBackendInitData data)
{
	VkApplicationInfo appCreateInfo{};
	appCreateInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appCreateInfo.pApplicationName = "Zn";
	appCreateInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appCreateInfo.pEngineName = "Zn";
	appCreateInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appCreateInfo.apiVersion = VK_API_VERSION_1_0;

	// Get the names of the Vulkan instance extensions needed to create a surface with SDL_Vulkan_CreateSurface
	SDL_Window* window = SDL_GetWindowFromID(data.window->GetSDLWindowID());

	u32 numExtensions = 0;
	SDL_Vulkan_GetInstanceExtensions(window, &numExtensions, nullptr);
	Vector<const char*> requiredExtensions(numExtensions);
	SDL_Vulkan_GetInstanceExtensions(window, &numExtensions, requiredExtensions.data());

	VkInstanceCreateInfo instanceCreateInfo{};
	instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceCreateInfo.pApplicationInfo = &appCreateInfo;	

	Vector<const char*> enabledLayers;

#if ZN_VK_VALIDATION_LAYERS
	// Request debug utils extension if validation layers are enabled.
	numExtensions += static_cast<u32>(kRequiredExtensions.size());
	requiredExtensions.insert(requiredExtensions.end(), kRequiredExtensions.begin(), kRequiredExtensions.end());

	if (VulkanValidation::SupportsValidationLayers())
	{
		enabledLayers.insert(enabledLayers.begin(), kValidationLayers.begin(), kValidationLayers.end());
		
		VkDebugUtilsMessengerCreateInfoEXT debugMessagesInfo = VulkanValidation::GetDebugMessengerCreateInfo();
		instanceCreateInfo.pNext = &debugMessagesInfo;
	}
#endif

	instanceCreateInfo.enabledExtensionCount = numExtensions;
	instanceCreateInfo.ppEnabledExtensionNames = requiredExtensions.data();
	instanceCreateInfo.enabledLayerCount = (u32) enabledLayers.size();
	instanceCreateInfo.ppEnabledLayerNames = enabledLayers.data();

	ZN_VK_CHECK(vkCreateInstance(&instanceCreateInfo, nullptr, &instance));

#if ZN_VK_VALIDATION_LAYERS
	VulkanValidation::InitializeDebugMessenger(instance);
#endif

	if (SDL_Vulkan_CreateSurface(window, instance, &surface) != SDL_TRUE)
	{
		_ASSERT(false);
		return false;
	}

	device = std::make_unique<VulkanDevice>();

	device->Initialize(window, instance, surface);

	return true;
}

void Zn::VulkanBackend::shutdown()
{
	device = nullptr;

	if (surface != VK_NULL_HANDLE)
	{
		vkDestroySurfaceKHR(instance, surface, nullptr);
		surface = VK_NULL_HANDLE;
	}

#if ZN_VK_VALIDATION_LAYERS
	VulkanValidation::DeinitializeDebugMessenger(instance);
#endif

	if (instance != VK_NULL_HANDLE)
	{
		vkDestroyInstance(instance, nullptr);
		instance = VK_NULL_HANDLE;
	}
}

bool Zn::VulkanBackend::begin_frame()
{
	device->BeginFrame();

	return true;
}

bool Zn::VulkanBackend::render_frame()
{
	device->Draw();

	return true;
}

bool Zn::VulkanBackend::end_frame()
{
	device->EndFrame();

	return true;
}

void Zn::VulkanBackend::on_window_resized()
{
	if (device != VK_NULL_HANDLE)
	{
		device->ResizeWindow();
	}
}

void Zn::VulkanBackend::on_window_minimized()
{
	if (device != VK_NULL_HANDLE)
	{
		device->OnWindowMinimized();
	}
}

void Zn::VulkanBackend::on_window_restored()
{
	if (device != VK_NULL_HANDLE)
	{
		device->OnWindowRestored();
	}
}

void Zn::VulkanBackend::set_camera(glm::vec3 position, glm::vec3 direction)
{
	if (device != VK_NULL_HANDLE)
	{
		device->camera_position = position;
		device->camera_direction = direction;
	}
}
#undef VMA_IMPLEMENTATION