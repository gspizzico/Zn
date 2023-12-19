#include <Engine/Engine.h>
#include <Application/Application.h>
#include <Core/Time/Time.h>
#include <Core/CoreAssert.h>

#include <Application/AppEventHandler.h>
#include <Engine/Application/AppEventHandlerImpl.h>

using namespace Zn;

DEFINE_STATIC_LOG_CATEGORY(LogEngine, ELogVerbosity::Log);

namespace
{
float GDeltaTime = 0.f;

} // namespace

void Zn::Engine::Create()
{
    AppEventHandler::SetEventHandler(new AppEventHandlerImpl());

    ZN_LOG(LogEngine, ELogVerbosity::Log, "Engine initialized.");
}

void Zn::Engine::Destroy()
{
}

void Zn::Engine::Tick(float deltaTime_)
{
}
