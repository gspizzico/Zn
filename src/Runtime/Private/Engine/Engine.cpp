#include <Engine/Engine.h>
#include <Application/Application.h>
#include <Core/Time/Time.h>

#include <Application/AppEventHandler.h>
#include <Engine/Application/AppEventHandlerImpl.h>

namespace
{
float GDeltaTime = 0.f;
}

int32 Zn::Engine::Launch()
{
    AppEventHandler::SetEventHandler(new AppEventHandlerImpl());

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
