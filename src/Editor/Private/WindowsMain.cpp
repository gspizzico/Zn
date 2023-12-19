#include <Core/CommandLine.h>
#include <Application/Application.h>
#include <ImGui/ImGuiApp.h>
#include <Engine/Engine.h>
#include <Editor.h>
#include <Rendering/Renderer.h>

#include <Core/Time/Time.h>

using namespace Zn;

int main(int argc_, char* args_[])
{
    CommandLine::Get().Initialize(argc_, args_);

    Application::Create();
    ImGuiApp::Create();
    Renderer::Create(RendererBackendType::Vulkan);
    Engine::Create();
    Editor::Create();

    Application& app = Application::Get();

    double lastFrameTime = Time::Seconds();

    double currentFrameTime = lastFrameTime;

    while (!app.WantsToExit())
    {
        currentFrameTime = Time::Seconds();

        float deltaTime = static_cast<float>(currentFrameTime - lastFrameTime);

        lastFrameTime = currentFrameTime;

        if (!app.ProcessOSEvents(deltaTime))
        {
            break;
        }

        Engine::Tick(deltaTime);
        Editor::Tick(deltaTime);

        Renderer::Get().Render(deltaTime,
                               [](float deltaTime_)
                               {
                                   ImGuiApp::BeginFrame();
                                   ImGuiApp::Tick(deltaTime_);
                                   ImGuiApp::EndFrame();
                               });
    }

    Editor::Destroy();
    Engine::Destroy();
    Renderer::Destroy();
    ImGuiApp::Destroy();
    Application::Destroy();

    return 0;
}
