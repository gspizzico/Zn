#include <Core/CommandLine.h>
#include <Application/Application.h>
#include <ImGui/ImGuiApp.h>
#include <Engine/Engine.h>
#include <Engine/RHI/RHIDevice.h>

#include <Core/Time/Time.h>

using namespace Zn;

int main(int argc_, char* args_[])
{
    CommandLine::Get().Initialize(argc_, args_);

    Application::Create();
    RHIDevice::Create();
    ImGuiApp::Create();
    Engine::Create();

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
    }

    Engine::Destroy();
    ImGuiApp::Destroy();
    RHIDevice::Destroy();
    Application::Destroy();

    return 0;
}
