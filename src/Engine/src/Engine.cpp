#include <Engine.h>
#include <Application.h>
#include <Time/Time.h>

#include <ApplicationEventHandler.h>
#include <Application/EngineApp.h>

namespace
{
float GDeltaTime = 0.f;
}

int32 Zn::Engine::Launch()
{
    ApplicationEventHandler::SetEventHandler(new EngineApp());

    Application& app = Application::Get();

    double lastFrameTime = Time::Seconds();

    double currentFrameTime = lastFrameTime;

    while (!app.WantsToExit())
    {
        currentFrameTime = Time::Seconds();

        GDeltaTime = static_cast<float>(currentFrameTime - lastFrameTime);

        lastFrameTime = currentFrameTime;

        if (!app.ProcessOSEvents(GDeltaTime))
        {
            break;
        }

        Engine::Tick(GDeltaTime);
    }

    return 0;
}

void Zn::Engine::Tick(float deltaTime_)
{
}
