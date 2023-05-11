#include <Znpch.h>

#include <ImGui/ImGuiWrapper.h>

#include <imgui.h>
#include <imgui_impl_sdl.h>
#include <imgui_impl_vulkan.h>

#include <SDL.h>
#include <d3d11.h>

#include <Core/Memory/Memory.h>
#include <Core/CommandLine.h>

using namespace Zn;

namespace
{
bool imgui_use_zn_allocator()
{
    return !CommandLine::Get().Param("-noimguialloc");
}

void* imgui_alloc(size_t sz, void*)
{
    return Zn::Allocators::New(sz);
}

void imgui_free(void* ptr, void*)
{
    Zn::Allocators::Delete(ptr);
}

bool GImGuiInitialized = false;
} // namespace

void Zn::imgui_initialize()
{
    _ASSERT(GImGuiInitialized == false);

    IMGUI_CHECKVERSION();

    if (imgui_use_zn_allocator())
    {
        ImGui::SetAllocatorFunctions(imgui_alloc, imgui_free);
    }

    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    (void) io;
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    // ImGui::StyleColorsLight();

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error
    // and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which
    // ImGui_ImplXXXX_NewFrame below will call.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    // io.Fonts->AddFontDefault();
    // io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    // io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    // io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    // io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
    // ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    // IM_ASSERT(font != NULL);

    GImGuiInitialized = true;
}

void Zn::imgui_shutdown()
{
    _ASSERT(GImGuiInitialized);

    // TODO: why is this here?
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
}

bool Zn::imgui_process_event(SDL_Event& event)
{
    return ImGui_ImplSDL2_ProcessEvent(&event);
}

bool Zn::imgui_begin_frame()
{
    if (GImGuiInitialized)
    {
        // Start the Dear ImGui frame
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
    }

    return GImGuiInitialized;
}

bool Zn::imgui_end_frame()
{
    if (GImGuiInitialized)
    {
        ImGui::EndFrame();
    }
    return GImGuiInitialized;
}