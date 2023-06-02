#include <Engine/Engine.h>
#include <Application/Application.h>
#include <Core/Time/Time.h>

#include <Application/AppEventHandler.h>
#include <Engine/Application/AppEventHandlerImpl.h>
#include <RHI/RHIDevice.h>

using namespace Zn;

namespace
{
float GDeltaTime = 0.f;
} // namespace

void Zn::Engine::Create()
{
    AppEventHandler::SetEventHandler(new AppEventHandlerImpl());
}

void Zn::Engine::Destroy()
{
}

void Zn::Engine::Tick(float deltaTime_)
{
}
