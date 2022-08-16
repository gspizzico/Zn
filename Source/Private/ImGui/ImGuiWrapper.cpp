#include <ImGui/ImGuiWrapper.h>

#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_sdl.h"
#include "ImGui/imgui_impl_dx11.h"

#include <SDL.h>
#include <d3d11.h>

#include <Core/Memory/Memory.h>
#include <Core/CommandLine.h>

#include <Core/Trace/Trace.h>

using namespace Zn;

bool ImGuiUseZnAllocator()
{
	return !CommandLine::Get().Param("-noimguialloc");
}

void* ImGuiAlloc(size_t sz, void*)
{
	return Zn::Allocators::New(sz);
}
void ImGuiFree(void* ptr, void*)
{
	Zn::Allocators::Delete(ptr);
}

void ImGuiWrapper::Initialize(SDL_Window* window, ID3D11Device* d3dDevice, ID3D11DeviceContext* d3dDeviceContext)
{
	IMGUI_CHECKVERSION();

	if (ImGuiUseZnAllocator())
	{
		ImGui::SetAllocatorFunctions(ImGuiAlloc, ImGuiFree);
	}

	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO(); (void) io;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsLight();

	// Setup Platform/Renderer backends
	ImGui_ImplSDL2_InitForD3D(window);
	ImGui_ImplDX11_Init(d3dDevice, d3dDeviceContext);

	// Load Fonts
	// - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
	// - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
	// - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
	// - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
	// - Read 'docs/FONTS.md' for more instructions and details.
	// - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
	//io.Fonts->AddFontDefault();
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
	//ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
	//IM_ASSERT(font != NULL);
}

void ImGuiWrapper::ProcessEvent(SDL_Event& event)
{
	ImGui_ImplSDL2_ProcessEvent(&event);
}

void ImGuiWrapper::NewFrame()
{
	// Start the Dear ImGui frame
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplSDL2_NewFrame();
	ImGui::NewFrame();

	//bool show = true;
	//ImGui::ShowDemoWindow(&show);
}

void ImGuiWrapper::EndFrame()
{
	ZN_TRACE_QUICKSCOPE();

	ImGui::Render();

	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void ImGuiWrapper::Shutdown()
{
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();
}