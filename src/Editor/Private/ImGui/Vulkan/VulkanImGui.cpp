#include <ImGui/RHIImGui.h>
#include <Core/CoreAssert.h>
#include <Application/Application.h>
#include <sdl/SDL.h>
#include <imgui/backends/imgui_impl_sdl.h>
#include <imgui/backends/imgui_impl_vulkan.h>

void Zn::RHIImGui::Initialize()
{
    WindowHandle windowHandle = Application::Get().GetWindowHandle();

    check(windowHandle.handle != nullptr);

    SDL_Window* window = SDL_GetWindowFromID(static_cast<uint32>(windowHandle.id));

    ImGui_ImplSDL2_InitForVulkan(window);
}

void Zn::RHIImGui::Shutdown()
{
    // ImGui_ImplVulkan_Shutdown();
}
