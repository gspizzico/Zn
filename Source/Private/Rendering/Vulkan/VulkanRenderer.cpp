#include "Znpch.h"
#include "Rendering/Vulkan/Vulkan.h"
#include "Rendering/Vulkan/VulkanRenderer.h"
#include "Rendering/Vulkan/VulkanUtils.h"
#include "Application/Window.h"

#include <Core/Log/LogMacros.h>

#include <algorithm>
#include <ImGui/ImGuiWrapper.h>

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

bool Zn::VulkanRenderer::initialize(RendererInitParams params)
{
	vk::ApplicationInfo appInfo{};
	appInfo.pApplicationName = "Zn";
	appInfo.pEngineName = "Zn";
	appInfo.apiVersion = VK_API_VERSION_1_0;

	// Get the names of the Vulkan instance extensions needed to create a surface with SDL_Vulkan_CreateSurface
	SDL_Window* window = SDL_GetWindowFromID(params.window->GetSDLWindowID());

	u32 numExtensions = 0;
	SDL_Vulkan_GetInstanceExtensions(window, &numExtensions, nullptr);
	Vector<const char*> requiredExtensions(numExtensions);
	SDL_Vulkan_GetInstanceExtensions(window, &numExtensions, requiredExtensions.data());

	vk::InstanceCreateInfo instanceCreateInfo{};
	instanceCreateInfo.pApplicationInfo = &appInfo;

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

	instance = vk::createInstance(instanceCreateInfo);

#if ZN_VK_VALIDATION_LAYERS
	VulkanValidation::InitializeDebugMessenger(instance);
#endif

	VkSurfaceKHR sdlSurface;
	if (SDL_Vulkan_CreateSurface(window, instance, &sdlSurface) != SDL_TRUE)
	{
		_ASSERT(false);
		return false;
	}

	surface = sdlSurface;

	device = std::make_unique<VulkanDevice>();

	device->Initialize(window, instance, surface);

	return true;
}

void Zn::VulkanRenderer::shutdown()
{
	if (!instance)
	{
		return;
	}

	device = nullptr;

	Zn::imgui_shutdown();

	if (surface)
	{
		instance.destroySurfaceKHR(surface);
	}

#if ZN_VK_VALIDATION_LAYERS
	VulkanValidation::DeinitializeDebugMessenger(instance);
#endif

	instance.destroy();
}

bool Zn::VulkanRenderer::begin_frame()
{
	Zn::imgui_begin_frame();

	device->BeginFrame();

	return true;
}

bool Zn::VulkanRenderer::render_frame(float deltaTime, std::function<void(float)> render)
{
	if (!begin_frame())
	{
		ZN_LOG(LogVulkan_2, ELogVerbosity::Error, "Failed to begin_frame.");
		return false;
	}

	if (render)
	{
		render(deltaTime);
	}

	device->Draw();

	if (!end_frame())
	{
		ZN_LOG(LogVulkan_2, ELogVerbosity::Error, "Failed to end_frame.");
	}

	return true;
}

bool Zn::VulkanRenderer::end_frame()
{
	Zn::imgui_end_frame();

	device->EndFrame();

	return true;
}

void Zn::VulkanRenderer::on_window_resized()
{
	if (device != VK_NULL_HANDLE)
	{
		device->ResizeWindow();
	}
}

void Zn::VulkanRenderer::on_window_minimized()
{
	if (device != VK_NULL_HANDLE)
	{
		device->OnWindowMinimized();
	}
}

void Zn::VulkanRenderer::on_window_restored()
{
	if (device != VK_NULL_HANDLE)
	{
		device->OnWindowRestored();
	}
}

void Zn::VulkanRenderer::set_camera(glm::vec3 position, glm::vec3 direction)
{
	if (device != VK_NULL_HANDLE)
	{
		device->cameraPosition = position;
		device->cameraDirection = direction;
	}
}
#undef VMA_IMPLEMENTATION