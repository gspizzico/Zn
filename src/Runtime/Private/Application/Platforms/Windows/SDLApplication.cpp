#include <Windows/SDLApplication.h>
#include <Windows/SDLWindow.h>
#include <Application/ApplicationInput.h>
#include <sdl/SDL.h>
#include <Core/IO/IO.h>
#include <filesystem>
#include <cmath>

#if WITH_IMGUI
    #include <Core/CommandLine.h>
    #include <Core/Memory/Memory.h>
    #include <imgui/imgui.h>
    #include <imgui/backends/imgui_impl_sdl.h>
#endif

using namespace Zn;

DEFINE_STATIC_LOG_CATEGORY(LogSDLApplication, ELogVerbosity::Log);

namespace
{
constexpr int32 SCREEN_WIDTH  = 1280;
constexpr int32 SCREEN_HEIGHT = 720;

bool GPlatformApplicationInitialized = false;

String GetIORootPath()
{
    std::filesystem::path executablePath(CommandLine::Get().GetExeArgument());

    // TODO: Fix path after refactor directory change
    return executablePath.parent_path().parent_path().parent_path().string();
}

#if WITH_IMGUI
bool ImGuiUseZnAllocator()
{
    return !Zn::CommandLine::Get().Param("-imgui.usedefaultmalloc");
}

void* ImGuiAlloc(sizet size_, void*)
{
    return Zn::Allocators::New(size_, Zn::MemoryAlignment::kDefaultAlignment);
}

void ImGuiFree(void* address_, void*)
{
    return Zn::Allocators::Delete(address_);
}
#endif
} // namespace

namespace Zn
{

void Platform_InitializeApplication()
{
    check(GPlatformApplicationInitialized == false);

    IO::Initialize(GetIORootPath());

    SDLApplication* application = new SDLApplication();

    application->Initialize();

    GPlatformApplicationInitialized = true;
}

void Platform_ShutdownApplication()
{
    check(GPlatformApplicationInitialized);

    SDLApplication* application = static_cast<SDLApplication*>(&Application::Get());

    application->Shutdown();
}

void SDLApplication::Initialize()
{
    ZN_TRACE_QUICKSCOPE();

    check(!isInitialized);

    if (SDL_InitSubSystem(SDL_INIT_VIDEO /*TODO: add more flags here*/) < 0)
    {
        ZN_LOG(LogSDLApplication, ELogVerbosity::Error, "SDL could not initialize. SDL_Error: %s", SDL_GetError());

        PlatformMisc::Exit(true);
    }

    title = "Zn-Engine";

    window = std::make_shared<SDLWindow>(SCREEN_WIDTH, SCREEN_HEIGHT, title);

    inputState = std::make_shared<InputState>();

#if WITH_IMGUI
    IMGUI_CHECKVERSION();

    if (ImGuiUseZnAllocator())
    {
        ImGui::SetAllocatorFunctions(ImGuiAlloc, ImGuiFree);
    }

    ImGui::CreateContext();

    // ImGuiIO& io = ImGui::GetIO();
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont()
    // to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion,
    // or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling
    // ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    // io.Fonts->AddFontDefault();
    // io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    // io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    // io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    // io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
    // ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    // IM_ASSERT(font != NULL);

    ZN_LOG(LogSDLApplication, ELogVerbosity::Log, "ImGui context initialized.");
#endif

    isInitialized = true;

    SetApplication(this);

    ZN_LOG(LogSDLApplication, ELogVerbosity::Log, "Application initialized.");
}

void SDLApplication::Shutdown()
{
    check(isInitialized);

    window = nullptr;

    inputState = nullptr;

#if WITH_IMGUI
    // TODO: This should be initialized by the renderer
    // ImGui_ImplSDL2_Shutdown();

    ImGui::DestroyContext();
#endif

    SDL_Quit();

    SetApplication(nullptr);
}

bool SDLApplication::ProcessOSEvents(float deltaTime_)
{
    check(isInitialized);

    deltaTime_ = std::max(deltaTime_, FLT_EPSILON);

    SDL_Event event;

    String titleString = title + String(" FPS: ") + std::to_string(static_cast<int32>(floor(1.f / deltaTime_)));
    window->SetTitle(titleString.c_str());

    inputState->events.clear();

    while (SDL_PollEvent(&event) != 0)
    {
#if WITH_IMGUI
        ImGui_ImplSDL2_ProcessEvent(&event);
#endif
        if (event.type == SDL_QUIT)
        {
            isExitRequested = true;
        }
        else if (event.type == SDL_WINDOWEVENT)
        {
            isExitRequested |= !window->ProcessEvent(event);
        }
        else if (event.type >= SDL_KEYDOWN && event.type <= SDL_CONTROLLERSENSORUPDATE)
        {
            inputState->events.push_back(event);
        }
    }

    return !isExitRequested;
}
WindowHandle SDLApplication::GetWindowHandle() const
{
    check(isInitialized);

    return window->GetHandle();
}
SharedPtr<InputState> SDLApplication::GetInputState() const
{
    return inputState;
}
void SDLApplication::RequestExit(cstring exitReason_)
{
    if (!isExitRequested)
    {
        ZN_LOG(LogSDLApplication, ELogVerbosity::Log, "Requested application exit. Reason: %s", exitReason_);

        isExitRequested = true;
    }
}
bool SDLApplication::WantsToExit() const
{
    return false;
}
} // namespace Zn
