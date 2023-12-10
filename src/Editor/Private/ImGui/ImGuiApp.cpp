#include <ImGui/ImGuiApp.h>
#include <Core/CommandLine.h>
#include <Core/Memory/Memory.h>
#include <Core/Log/LogMacros.h>
#include <Application/Application.h>
#include <RHI/RHIDevice.h>

#include <imgui/imgui.h>

#if PLATFORM_WINDOWS
#include <Windows/Application/SDLApplication.h>
#include <imgui/backends/imgui_impl_sdl.h>
#endif // PLATFORM_WINDOWS

#if WITH_VULKAN
#include <imgui/backends/imgui_impl_vulkan.h>
#include <RHI/Vulkan/VulkanContext.h>
#endif // WITH_VULKAN

using namespace Zn;

DEFINE_STATIC_LOG_CATEGORY(LogImGui, ELogVerbosity::Log);

namespace
{
bool ImGuiUseZnAllocator()
{
    return !CommandLine::Get().Param("-imgui.usedefaultmalloc");
}

void* ImGuiAlloc(sizet size_, void*)
{
    return Zn::Allocators::New(size_, Zn::MemoryAlignment::DefaultAlignment);
}

void ImGuiFree(void* address_, void*)
{
    return Zn::Allocators::Delete(address_);
}

void ProcessSDLEvent(const SDL_Event* event_)
{
    ImGui_ImplSDL2_ProcessEvent(event_);
}

EventHandle GProcessSDLEventHandle;
} // namespace

#if WITH_VULKAN
vk::DescriptorPool GImGuiDescriptorPool {nullptr};
#endif

bool GImGuiInitialized = false;

TMulticastEvent<float> ImGuiApp::OnTick {};

void ImGuiApp::Create()
{
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

#if PLATFORM_WINDOWS
    Application& app = Application::Get();

    SDLApplication* sdlApp = static_cast<SDLApplication*>(&app);

    GProcessSDLEventHandle = sdlApp->externalEventProcessor.Bind(cpp::bind(&ProcessSDLEvent));

#if WITH_VULKAN
    WindowHandle windowHandle = app.GetWindowHandle();

    check(windowHandle.handle != nullptr);

    SDL_Window* window = SDL_GetWindowFromID(static_cast<uint32>(windowHandle.id));

    ImGui_ImplSDL2_InitForVulkan(window);
#endif // WITH_VULKAN

#endif // PLATFORM_WINDOWS

#if WITH_VULKAN
    VulkanContext& vkContext = VulkanContext::Get();

    Vector<vk::DescriptorPoolSize> imguiPoolSizes = {{vk::DescriptorType::eSampler, 1000},
                                                     {vk::DescriptorType::eCombinedImageSampler, 1000},
                                                     {vk::DescriptorType::eSampledImage, 1000},
                                                     {vk::DescriptorType::eStorageImage, 1000},
                                                     {vk::DescriptorType::eUniformTexelBuffer, 1000},
                                                     {vk::DescriptorType::eStorageTexelBuffer, 1000},
                                                     {vk::DescriptorType::eUniformBuffer, 1000},
                                                     {vk::DescriptorType::eStorageBuffer, 1000},
                                                     {vk::DescriptorType::eUniformBufferDynamic, 1000},
                                                     {vk::DescriptorType::eStorageBufferDynamic, 1000},
                                                     {vk::DescriptorType::eInputAttachment, 1000}};

    vk::DescriptorPoolCreateInfo imguiPoolCreateInfo {
        .flags   = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        .maxSets = 1000,
    };

    imguiPoolCreateInfo.setPoolSizes(imguiPoolSizes);

    GImGuiDescriptorPool = vkContext.device.createDescriptorPool(imguiPoolCreateInfo);

    // this initializes imgui for Vulkan
    ImGui_ImplVulkan_InitInfo imguiInitInfo {.Instance       = vkContext.instance,
                                             .PhysicalDevice = vkContext.gpu.gpu,
                                             .Device         = vkContext.device,
                                             .Queue          = vkContext.graphicsQueue,
                                             .DescriptorPool = GImGuiDescriptorPool,
                                             .MinImageCount  = 3,
                                             .ImageCount     = 3,
                                             .MSAASamples    = VK_SAMPLE_COUNT_1_BIT};

    ImGui_ImplVulkan_Init(&imguiInitInfo, vkContext.mainRenderPass);

    // Upload Fonts

    vk::CommandBuffer& cmdBuffer = vkContext.graphicsCmdContext.commandBuffers[0];
    cmdBuffer.reset();

    cmdBuffer.begin(vk::CommandBufferBeginInfo {.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});

    ImGui_ImplVulkan_CreateFontsTexture(cmdBuffer);

    cmdBuffer.end();

    vkContext.graphicsQueue.submit(vk::SubmitInfo {
        .commandBufferCount = 1,
        .pCommandBuffers    = &cmdBuffer,
    });

    vkContext.device.waitIdle();

    ImGui_ImplVulkan_DestroyFontUploadObjects();
#endif

    GImGuiInitialized = true;

    ZN_LOG(LogImGui, ELogVerbosity::Log, "ImGui context initialized.");
}

void ImGuiApp::Destroy()
{
#if PLATFORM_WINDOWS
    if (GProcessSDLEventHandle)
    {
        Application& app = Application::Get();

        SDLApplication* sdlApp = static_cast<SDLApplication*>(&app);

        sdlApp->externalEventProcessor.Unbind(GProcessSDLEventHandle);
    }
#endif

#if WITH_VULKAN
    if (GImGuiDescriptorPool)
    {
        VulkanContext& vkContext = VulkanContext::Get();

        vkContext.device.destroyDescriptorPool(GImGuiDescriptorPool);
        GImGuiDescriptorPool = nullptr;
    }
    ImGui_ImplVulkan_Shutdown();
#endif // WITH_VULKAN

    ImGui::DestroyContext();

    GImGuiInitialized = false;
}

void ImGuiApp::BeginFrame()
{
    if (GImGuiInitialized)
    {
#if WITH_VULKAN
        ImGui_ImplVulkan_NewFrame();
#endif
#if PLATFORM_WINDOWS
        ImGui_ImplSDL2_NewFrame();
#endif
        ImGui::NewFrame();
    }
}

void ImGuiApp::EndFrame()
{
    if (GImGuiInitialized)
    {
        ImGui::EndFrame();
    }
}

void ImGuiApp::Tick(float deltaTime_)
{
    OnTick.Broadcast(deltaTime_);
}
