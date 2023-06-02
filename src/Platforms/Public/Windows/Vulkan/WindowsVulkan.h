#pragma once

#include <Core/CoreTypes.h>
#include <Engine/RHI/Vulkan/Vulkan.h>

namespace Zn
{
struct WindowsVulkan
{
    static Vector<cstring> GetInstanceExtensions();
    static vk::SurfaceKHR  CreateSurface(vk::Instance instance_);
    static void            GetSurfaceDrawableSize(uint32& outWidth_, uint32& outHeight_);
};

using PlatformVulkan = WindowsVulkan;
} // namespace Zn
