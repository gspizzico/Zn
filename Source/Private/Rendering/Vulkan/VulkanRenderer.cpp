#include "Znpch.h"
#include "Rendering/Vulkan/Vulkan.h"
#include "Rendering/Vulkan/VulkanRenderer.h"
#include "Rendering/Vulkan/VulkanUtils.h"
#include "Application/Window.h"

#include <Core/Log/LogMacros.h>

#include <algorithm>
#include <ImGui/ImGuiWrapper.h>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

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
		Zn::Vector<vk::LayerProperties> availableLayers = vk::enumerateInstanceLayerProperties();

		return std::any_of(availableLayers.begin(), availableLayers.end(), [](const vk::LayerProperties& it)
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

	ELogVerbosity VkMessageSeverityToZnVerbosity(vk::DebugUtilsMessageSeverityFlagBitsEXT severity)
	{
		switch (severity)
		{
			case vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose:
			return ELogVerbosity::Verbose;
			case vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo:
			return ELogVerbosity::Log;
			case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning:
			return ELogVerbosity::Warning;
			case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError:
			default:
			return ELogVerbosity::Error;
		}
	}

	VKAPI_ATTR VkBool32 VKAPI_CALL OnDebugMessage(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
									  VkDebugUtilsMessageTypeFlagsEXT type,
									  const VkDebugUtilsMessengerCallbackDataEXT* data,
									  void* userData)
	{
		ELogVerbosity verbosity = VkMessageSeverityToZnVerbosity(vk::DebugUtilsMessageSeverityFlagBitsEXT(severity));

		const String& messageType = vk::to_string(vk::DebugUtilsMessageTypeFlagsEXT(type));

		ZN_LOG(LogVulkanValidation_2, verbosity, "[%s] %s", messageType.c_str(), data->pMessage);

		if (verbosity >= ELogVerbosity::Error)
		{
			__debugbreak();
		}

		return VK_FALSE;
	}

	vk::DebugUtilsMessengerCreateInfoEXT GetDebugMessengerCreateInfo()
	{
		vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo
		{
#if ZN_VK_VALIDATION_VERBOSE
		.messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo | vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
#else
		.messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
#endif
		.messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
		.pfnUserCallback = VulkanValidation::OnDebugMessage
		};

		return debugCreateInfo;
	}

	vk::DebugUtilsMessengerEXT GDebugMessenger{};

	void InitializeDebugMessenger(vk::Instance instance)
	{
		GDebugMessenger = instance.createDebugUtilsMessengerEXT(GetDebugMessengerCreateInfo());
	}

	void DeinitializeDebugMessenger(vk::Instance instance)
	{
		if (GDebugMessenger)
		{
			instance.destroyDebugUtilsMessengerEXT(GDebugMessenger);
		}
	}
}
#endif

bool Zn::VulkanRenderer::initialize(RendererInitParams params)
{
	// Initialize vk::DynamicLoader - It's needed to call .dll functions.
	vk::DynamicLoader dynamicLoader;
	auto pGetInstance = dynamicLoader.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
	VULKAN_HPP_DEFAULT_DISPATCHER.init(pGetInstance);

	vk::ApplicationInfo appInfo
	{
		.pApplicationName = "Zn",
		.pEngineName = "Zn",
		.apiVersion = VK_API_VERSION_1_0,
	};

	// Get the names of the Vulkan instance extensions needed to create a surface with SDL_Vulkan_CreateSurface
	SDL_Window* window = SDL_GetWindowFromID(params.window->GetSDLWindowID());

	u32 numExtensions = 0;
	SDL_Vulkan_GetInstanceExtensions(window, &numExtensions, nullptr);
	Vector<const char*> requiredExtensions(numExtensions);
	SDL_Vulkan_GetInstanceExtensions(window, &numExtensions, requiredExtensions.data());

	vk::InstanceCreateInfo instanceCreateInfo
	{
		.pApplicationInfo = &appInfo
	};

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
	VULKAN_HPP_DEFAULT_DISPATCHER.init(instance);

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