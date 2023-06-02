#include <ApplicationEventHandler.h>
#include <CoreAssert.h>

using namespace Zn;

namespace
{
ApplicationEventHandler* GAppEventHandler = nullptr;
}

ApplicationEventHandler& Zn::ApplicationEventHandler::Get()
{
    checkMsg(GAppEventHandler, "Trying to get Application Event Handler before it's initialized");
    return *GAppEventHandler;
}

void Zn::ApplicationEventHandler::SetEventHandler(ApplicationEventHandler* eventHandler_)
{
    check(GAppEventHandler == nullptr || eventHandler_ == nullptr);

    if (GAppEventHandler)
    {
        delete GAppEventHandler;
    }

    GAppEventHandler = eventHandler_;
}
