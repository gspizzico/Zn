#include <Windows/Vulkan/WindowsVulkan.h>
#include <Application/Application.h>
#include <Core/CoreAssert.h>

#include <sdl/SDL_vulkan.h>

using namespace Zn;

namespace
{
SDL_Window* GetSDLWindow()
{
    WindowHandle windowHandle = Application::Get().GetWindowHandle();

    check(windowHandle.handle != nullptr);

    return SDL_GetWindowFromID(static_cast<uint32>(windowHandle.id));
}
} // namespace

Vector<cstring> Zn::WindowsVulkan::GetInstanceExtensions()
{
    SDL_Window* window = GetSDLWindow();

    uint32 numExtensions = 0;

    SDL_Vulkan_GetInstanceExtensions(window, &numExtensions, nullptr);

    Vector<cstring> outExtensions(numExtensions);

    SDL_Vulkan_GetInstanceExtensions(window, &numExtensions, outExtensions.data());

    return outExtensions;
}

vk::SurfaceKHR Zn::WindowsVulkan::CreateSurface(vk::Instance instance_)
{
    SDL_Window* window = GetSDLWindow();

    VkSurfaceKHR outSurface;
    if (SDL_Vulkan_CreateSurface(window, instance_, &outSurface) != SDL_TRUE)
    {
        PlatformMisc::Exit(true);
    }

    return outSurface;
}

void Zn::WindowsVulkan::GetSurfaceDrawableSize(uint32& outWidth, uint32& outHeight_)
{
    SDL_Window* window = GetSDLWindow();
    int32       w, h;
    SDL_Vulkan_GetDrawableSize(window, &w, &h);

    outWidth   = static_cast<uint32>(w);
    outHeight_ = static_cast<uint32>(h);
}
