#include "Znpch.h"
#include "Rendering/RHI/RHI.h"
#include "Rendering/Vulkan/VulkanRenderer.h"
#include "Application/Window.h"

#include <Core/Log/LogMacros.h>

#include <algorithm>
#include <ImGui/ImGuiWrapper.h>

using namespace Zn;

bool Zn::VulkanRenderer::initialize(RendererInitParams params)
{
    // Get the names of the Vulkan instance extensions needed to create a surface with SDL_Vulkan_CreateSurface
    SDL_Window* window = SDL_GetWindowFromID(params.window->GetSDLWindowID());

    device = std::make_unique<VulkanDevice>();

    device->Initialize(window);

    return true;
}

void Zn::VulkanRenderer::shutdown()
{
    device = nullptr;

    Zn::imgui_shutdown();
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
        ZN_LOG(LogVulkan, ELogVerbosity::Error, "Failed to begin_frame.");
        return false;
    }

    if (render)
    {
        render(deltaTime);
    }

    device->Draw();

    if (!end_frame())
    {
        ZN_LOG(LogVulkan, ELogVerbosity::Error, "Failed to end_frame.");
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
        device->cameraPosition  = position;
        device->cameraDirection = direction;
    }
}
