#include <Application/AppEventHandler.h>
#include <Core/CoreAssert.h>

using namespace Zn;

namespace
{
AppEventHandler* GAppEventHandler = nullptr;
}

AppEventHandler& Zn::AppEventHandler::Get()
{
    checkMsg(GAppEventHandler, "Trying to get App Event Handler before it's initialized");
    return *GAppEventHandler;
}

void Zn::AppEventHandler::SetEventHandler(AppEventHandler* eventHandler_)
{
    check(GAppEventHandler == nullptr || eventHandler_ == nullptr);

    if (GAppEventHandler)
    {
        delete GAppEventHandler;
    }

    GAppEventHandler = eventHandler_;
}
