#include "Rendering/RHI/RHI.h"
#include "Rendering/Vulkan/VulkanRenderer.h"
#include <Application/Application.h>

#include <Core/Log/LogMacros.h>

#include <algorithm>

using namespace Zn;

bool Zn::VulkanRenderer::Initialize()
{
    // Get the names of the Vulkan instance extensions needed to create a surface with SDL_Vulkan_CreateSurface
    const uint32 windowHandle = static_cast<uint32>(Application::Get().GetWindowHandle().id);
    SDL_Window*  window       = SDL_GetWindowFromID(windowHandle);

    device = std::make_unique<VulkanDevice>();

    device->Initialize(window);

    return true;
}

void Zn::VulkanRenderer::Shutdown()
{
    device = nullptr;
}

bool Zn::VulkanRenderer::BeginFrame()
{
    device->BeginFrame();

    return true;
}

bool Zn::VulkanRenderer::Render(float deltaTime, std::function<void(float)> render)
{
    if (!BeginFrame())
    {
        ZN_LOG(LogVulkan, ELogVerbosity::Error, "Failed to begin_frame.");
        return false;
    }

    if (render)
    {
        render(deltaTime);
    }

    device->Draw();

    if (!EndFrame())
    {
        ZN_LOG(LogVulkan, ELogVerbosity::Error, "Failed to end_frame.");
    }

    return true;
}

bool Zn::VulkanRenderer::EndFrame()
{
    device->EndFrame();

    return true;
}

void Zn::VulkanRenderer::OnWindowResized(uint32 width_, uint32 height_)
{
    if (device != VK_NULL_HANDLE)
    {
        device->ResizeWindow();
    }
}

void Zn::VulkanRenderer::OnWindowMinimized()
{
    if (device != VK_NULL_HANDLE)
    {
        device->OnWindowMinimized();
    }
}

void Zn::VulkanRenderer::OnWindowRestored()
{
    if (device != VK_NULL_HANDLE)
    {
        device->OnWindowRestored();
    }
}

void Zn::VulkanRenderer::SetCamera(const ViewInfo& viewInfo)
{
    if (device)
    {
        device->cameraView = viewInfo;
    }
}

void Zn::VulkanRenderer::SetLight(glm::vec3 light, float distance, float intensity)
{
    if (device)
    {
        device->light          = light;
        device->lightDistance  = distance;
        device->lightIntensity = intensity;
    }
}
