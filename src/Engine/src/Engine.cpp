#include <Engine.h>

#include <ApplicationEventHandler.h>
#include <Application/EngineApp.h>

int32 Zn::Engine::Launch()
{
    ApplicationEventHandler::SetEventHandler(new EngineApp());

    return 0;
}
