#include <RHI/Vulkan/VulkanGPU.h>
#include <Core/CoreAssert.h>

using namespace Zn;

namespace
{
const Vector<cstring> GDeviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

struct QueueFamilyIndices
{
    std::optional<uint32> graphics;
    std::optional<uint32> present;
    std::optional<uint32> compute;
    std::optional<uint32> transfer;
};

QueueFamilyIndices GetQueueFamilyIndices(vk::PhysicalDevice device_, vk::SurfaceKHR surface_)
{
    QueueFamilyIndices outIndices;

    Vector<vk::QueueFamilyProperties> queueFamilies = device_.getQueueFamilyProperties();

    for (u32 idx = 0; idx < queueFamilies.size(); ++idx)
    {
        const vk::QueueFamilyProperties& queueFamily = queueFamilies[idx];

        if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
        {
            outIndices.graphics = idx;
        }

        if (queueFamily.queueFlags & vk::QueueFlagBits::eCompute)
        {
            outIndices.compute = idx;
        }

        if (queueFamily.queueFlags & vk::QueueFlagBits::eTransfer)
        {
            outIndices.transfer = idx;
        }

        // tbd: We could enforce that Graphics and Present are in the same queue but is not mandatory.
        if (device_.getSurfaceSupportKHR(idx, surface_) == VK_TRUE)
        {
            outIndices.present = idx;
        }
    }

    return outIndices;
}

bool HasRequiredDeviceExtensions(vk::PhysicalDevice device_)
{
    Vector<vk::ExtensionProperties> availableExtensions = device_.enumerateDeviceExtensionProperties();

    static const Set<String> kRequiredDeviceExtensions(GDeviceExtensions.begin(), GDeviceExtensions.end());

    auto numFoundExtensions = std::count_if(availableExtensions.begin(),
                                            availableExtensions.end(),
                                            [](const vk::ExtensionProperties& extension)
                                            {
                                                return kRequiredDeviceExtensions.contains(extension.extensionName);
                                            });

    return kRequiredDeviceExtensions.size() == numFoundExtensions;
}
std::pair<vk::PhysicalDevice, QueueFamilyIndices> SelectPhysicalDevice(vk::Instance instance_, vk::SurfaceKHR surface_)
{
    Vector<vk::PhysicalDevice> devices = instance_.enumeratePhysicalDevices();
    Vector<QueueFamilyIndices> queueFamilyIndices(devices.size());

    int32  selectedIndex = std::numeric_limits<i32>::max();
    uint32 maxScore      = 0;
    int32  numDevices    = static_cast<int32>(devices.size());

    for (int32 idx = 0; idx < numDevices; ++idx)
    {
        vk::PhysicalDevice device = devices[idx];

        uint32 deviceScore = 0;

        queueFamilyIndices[idx] = GetQueueFamilyIndices(device, surface_);

        const bool hasGraphicsQueue = queueFamilyIndices[idx].graphics.has_value();

        const bool hasRequiredExtensions = HasRequiredDeviceExtensions(device);

        if (hasGraphicsQueue && hasRequiredExtensions)
        {
            vk::PhysicalDeviceProperties deviceProperties = device.getProperties();
            vk::PhysicalDeviceFeatures   deviceFeatures   = device.getFeatures();

            if (deviceProperties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
            {
                deviceScore += 1000;
            }

            // Texture size influences quality
            deviceScore += deviceProperties.limits.maxImageDimension2D;

            // vkCmdDrawIndirect support
            if (deviceFeatures.multiDrawIndirect)
            {
                deviceScore += 500;
            }

            // TODO: Add more criteria to choose GPU.
        }

        if (deviceScore > maxScore && deviceScore != 0)
        {
            maxScore      = deviceScore;
            selectedIndex = idx;
        }
    }

    if (selectedIndex != std::numeric_limits<size_t>::max())
    {
        return {devices[selectedIndex], queueFamilyIndices[selectedIndex]};
    }
    else
    {
        return {VK_NULL_HANDLE, {}};
    }
}
} // namespace

VulkanGPU::VulkanGPU(vk::Instance instance_, vk::SurfaceKHR surface_)
{
    auto physicalDevice = SelectPhysicalDevice(instance_, surface_);

    check(physicalDevice.first);

    gpu           = physicalDevice.first;
    features      = gpu.getFeatures();
    graphicsQueue = physicalDevice.second.graphics.value_or(u32_max);
    presentQueue  = physicalDevice.second.present.value_or(u32_max);
    computeQueue  = physicalDevice.second.compute.value_or(u32_max);
    transferQueue = physicalDevice.second.transfer.value_or(u32_max);

    if (gpu.getSurfaceFormatsKHR(surface_).size() == 0 || gpu.getSurfacePresentModesKHR(surface_).size() == 0)
    {
        PlatformMisc::Exit(true);
    }
}

VulkanGPU ::~VulkanGPU()
{
}

vk::Device Zn::VulkanGPU::CreateDevice()
{
    static const float kQueuePriority = 1.f;

    vk::DeviceQueueCreateInfo queueCreateInfo[2] {
        {
            .queueFamilyIndex = graphicsQueue,
            .queueCount       = 1,
            .pQueuePriorities = &kQueuePriority,
        },
        {
            .queueFamilyIndex = presentQueue,
            .queueCount       = 1,
            .pQueuePriorities = &kQueuePriority,
        },
    };

    vk::DeviceCreateInfo deviceCreateInfo {
        .queueCreateInfoCount = ArraySize(queueCreateInfo),
        .pQueueCreateInfos    = ArrayData(queueCreateInfo),
    };

    deviceCreateInfo.setPEnabledExtensionNames(GDeviceExtensions);
    deviceCreateInfo.setPEnabledFeatures(&features);

    return gpu.createDevice(deviceCreateInfo);
}
