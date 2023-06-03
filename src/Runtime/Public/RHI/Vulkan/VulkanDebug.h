#pragma once
#include <RHI/Vulkan/Vulkan.h>
#include <Core/CoreTypes.h>

#if ZN_VK_VALIDATION_LAYERS
namespace Zn::VulkanValidation
{
static inline vk::DebugUtilsMessengerEXT GDebugMessenger {};
static inline const Vector<cstring>      GValidationExtensions = {VK_EXT_DEBUG_UTILS_EXTENSION_NAME};
static inline const Vector<cstring>      GValidationLayers     = {"VK_LAYER_KHRONOS_validation"};

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

VKAPI_ATTR VkBool32 VKAPI_CALL OnDebugMessage(VkDebugUtilsMessageSeverityFlagBitsEXT      severity,
                                              VkDebugUtilsMessageTypeFlagsEXT             type,
                                              const VkDebugUtilsMessengerCallbackDataEXT* data,
                                              void*                                       userData)
{
    ELogVerbosity verbosity = VkMessageSeverityToZnVerbosity(vk::DebugUtilsMessageSeverityFlagBitsEXT(severity));

    const String& messageType = vk::to_string(vk::DebugUtilsMessageTypeFlagsEXT(type));

    ZN_LOG(LogVulkanValidation, verbosity, "[%s] %s", messageType.c_str(), data->pMessage);

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
        .messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo | vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
                           vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
#else
        .messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                           vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
#endif
        .messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                       vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
        .pfnUserCallback = VulkanValidation::OnDebugMessage
    };

    return debugCreateInfo;
}

void InitializeDebugMessenger(vk::Instance instance_)
{
    check(!GDebugMessenger);

    GDebugMessenger = instance_.createDebugUtilsMessengerEXT(GetDebugMessengerCreateInfo());
}

void DeinitializeDebugMessenger(vk::Instance instance_)
{
    if (GDebugMessenger)
    {
        instance_.destroyDebugUtilsMessengerEXT(GDebugMessenger);
        GDebugMessenger = nullptr;
    }
}

bool SupportsValidationLayers()
{
    Vector<vk::LayerProperties> availableLayers = vk::enumerateInstanceLayerProperties();

    return std::any_of(availableLayers.begin(),
                       availableLayers.end(),
                       [](const vk::LayerProperties& it)
                       {
                           for (const auto& layerName : GValidationLayers)
                           {
                               if (strcmp(it.layerName, layerName) == 0)
                               {
                                   return true;
                               }
                           }

                           return false;
                       });
}

void InitializeInstanceForDebug(vk::InstanceCreateInfo& outCreateInfo_, Vector<cstring>& outInstanceExtensions_)
{
    // Request debug utils extension if validation layers are enabled.
    outInstanceExtensions_.insert(outInstanceExtensions_.end(), GValidationExtensions.begin(), GValidationExtensions.end());

    if (VulkanValidation::SupportsValidationLayers())
    {
        outCreateInfo_.setPEnabledLayerNames(GValidationLayers);
    }
}
} // namespace Zn::VulkanValidation
#endif
